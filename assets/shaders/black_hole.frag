#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec3 RayDir;

// Spacetime Geometry & Physical Metrics
uniform vec3 uCameraPos;
uniform vec3 uBhCenter;
uniform float uBoundRadius;
uniform float uSchwRadius;      // r_s = 2GM/c^2
uniform float uHorizonRadius;   // r_+
uniform float uPhotonRadius;    // r_ph
uniform float uShadowRadius;    // b_crit = sqrt(27)/2 * r_s
uniform float uSpinParam;       // a* in [-1.0, 1.0]
uniform vec3 uSpinAxis;         // Normalized spin axis vector

// Accretion Disk Physics (Novikov-Thorne)
uniform bool uHasDisk;
uniform float uDiskInnerRadius; // r_in = r_isco
uniform float uDiskOuterRadius; // r_out
uniform float uDiskPeakTemp;    // T_peak in Kelvin
uniform float uSimTime;

// Visual Mode & Background Skybox
uniform int uVisualMode;        // 0: Normal, 1: Realistic, 2: Cinematic
uniform sampler2D uSkyboxTex;
uniform bool uHasSkybox;

const float PI = 3.14159265358979323846;
const float TWO_PI = 6.28318530717958647692;

// ── Hash & Procedural 3D Noise for Disk Keplerian Turbulence ─────────────────
float hash31(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.1, 0.1));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

    float x00 = mix(n000, n100, u.x);
    float x10 = mix(n010, n110, u.x);
    float x01 = mix(n001, n101, u.x);
    float x11 = mix(n011, n111, u.x);

    float y0 = mix(x00, x10, u.y);
    float y1 = mix(x01, x11, u.y);

    return mix(y0, y1, u.z);
}

float fbmDisk(vec3 p) {
    float v = 0.0;
    float a = 0.55;
    vec3 shift = vec3(100.0);
    for (int i = 0; i < 3; ++i) {
        v += a * noise3D(p);
        p = p * 2.2 + shift;
        a *= 0.48;
    }
    return v;
}

// ── Physical Chromaticity Palette: Fiery Ember to Electric Diamond Cyan ───────
vec3 temperatureToColor(float T_kelvin) {
    float T = clamp(T_kelvin, 800.0, 36000.0);

    vec3 col;
    if (T < 2400.0) {
        // Deep smoldering ruby red to dark ember
        float t = (T - 800.0) / 1600.0;
        col = mix(vec3(0.60, 0.04, 0.01), vec3(0.95, 0.22, 0.02), t);
    } else if (T < 4500.0) {
        // Fiery amber to rich golden flame
        float t = (T - 2400.0) / 2100.0;
        col = mix(vec3(0.95, 0.22, 0.02), vec3(1.00, 0.65, 0.10), t);
    } else if (T < 7800.0) {
        // Incandescent solar gold to pure white
        float t = (T - 4500.0) / 3300.0;
        col = mix(vec3(1.00, 0.65, 0.10), vec3(1.00, 0.96, 0.90), t);
    } else if (T < 15000.0) {
        // Radiant stellar white to crisp electric diamond cyan
        float t = (T - 7800.0) / 7200.0;
        col = mix(vec3(1.00, 0.96, 0.90), vec3(0.68, 0.88, 1.20), t);
    } else {
        // High-energy extreme relativistic ultraviolet-blue
        float t = clamp((T - 15000.0) / 21000.0, 0.0, 1.0);
        col = mix(vec3(0.68, 0.88, 1.20), vec3(0.48, 0.72, 1.40), t);
    }

    return col;
}

// ── Convert 3D Direction to Equirectangular Skybox UV Coordinates ─────────────
vec2 dirToSkyboxUV(vec3 dir) {
    vec3 d = normalize(dir);
    float u = fract(1.0 - atan(d.z, d.x) / TWO_PI);
    float v = clamp(0.5 - asin(clamp(d.y, -1.0, 1.0)) / PI, 0.001, 0.999);
    return vec2(u, v);
}

// ── Background Skybox Sampling with Radiometric Linearization ────────────────
vec3 sampleSkyboxColor(vec3 dir) {
    if (!uHasSkybox) return vec3(0.0);
    vec2 uv = dirToSkyboxUV(dir);
    vec3 raw = texture(uSkyboxTex, uv).rgb;
    // Linearize from sRGB to radiometric space so stars shine with authentic intensity
    return pow(raw, vec3(2.2)) * 1.5;
}

// ── Relativistic Geodesic Acceleration: a = d^2 x / d lambda^2 ────────────────
// Exact Schwarzschild spatial acceleration + Kerr gravitomagnetic frame-dragging
vec3 computeGeodesicAcceleration(vec3 pos, vec3 dir, float rs, float spinParam, vec3 spinNorm) {
    float rSq = dot(pos, pos);
    float r = sqrt(rSq);
    float r3 = rSq * r;
    float r5 = r3 * rSq;

    // Exact Schwarzschild spatial acceleration: a = -1.5 * rs * (h^2 / r^5) * r
    vec3 hVec = cross(pos, dir);
    float hSq = dot(hVec, hVec);
    vec3 aSchw = -1.5 * rs * (hSq / max(r5, 1e-10)) * pos;

    // Kerr Gravitomagnetic Frame-Dragging Acceleration:
    // B_GM = (rs^2 * a* / (4 * r^3)) * [ 3(S_hat . r_hat) * r_hat - S_hat ]
    // a_Kerr = 2 * (v x B_GM)
    vec3 aKerr = vec3(0.0);
    if (abs(spinParam) > 0.001 && r > 1e-4) {
        vec3 rNorm = pos / r;
        float sDotR = dot(spinNorm, rNorm);
        vec3 bGm = (0.25 * rs * rs * spinParam / max(r3, 1e-8)) * (3.0 * sDotR * rNorm - spinNorm);
        aKerr = 2.0 * cross(dir, bGm);
    }

    return aSchw + aKerr;
}

// ── Evaluate Accretion Disk Emission at a Given Point in the Disk Plane ───────
void sampleDiskAtPoint(
    vec3 hitPos,
    vec3 rayDir,
    float rs,
    float rIn,
    float rOut,
    vec3 spinNorm,
    inout vec3 diskRadiance,
    inout float diskAlpha
) {
    float r = length(hitPos);
    if (r < rIn * 0.98 || r > rOut * 1.02) return;

    float rNorm = r / rIn;
    float flare = sqrt(max(0.0, 1.0 - 1.0 / sqrt(rNorm)));
    // Astrophysical scale height: very thin near ISCO, gently flaring outward
    float diskH = r * (0.015 * flare + 0.0035);

    // Path length through disk inclined at angle theta_inc: ds = 2H / |cos(theta_inc)|
    float cosInc = abs(dot(rayDir, spinNorm));
    float dsPath = (2.0 * diskH) / max(cosInc, 0.035);

    // Astrophysical Novikov-Thorne Temperature Profile:
    // T(r) peaks near 1.78 r_in and smoothly falls off as r^(-3/4)
    float fPeak = 0.4886; // value of (1/rNorm^3)^0.25 * (1 - 1/sqrt(rNorm))^0.25 at rNorm = 1.778
    float fR = pow(1.0 / (rNorm * rNorm * rNorm), 0.25) * pow(max(0.0, 1.0 - 1.0 / sqrt(rNorm)), 0.25);
    float tNorm = clamp(fR / fPeak, 0.0, 1.0);
    float localTemp = mix(1800.0, 14000.0, tNorm);

    // Relativistic Keplerian Orbital Motion in Kerr Spacetime
    float rg = rs * 0.5;
    float vBeta = clamp(sqrt(rg / r) / (1.0 + uSpinParam * pow(rg / r, 1.5)), 0.0, 0.85);
    vec3 vDir = normalize(cross(spinNorm, hitPos));

    // Relativistic Doppler Factor: delta = 1 / (gamma * (1 - beta . n))
    vec3 toObs = -rayDir;
    float betaDotN = clamp(vBeta * dot(vDir, toObs), -0.85, 0.85);
    float gamma = 1.0 / sqrt(1.0 - vBeta * vBeta);
    float dopplerFactor = 1.0 / (gamma * (1.0 - betaDotN));

    // Gravitational Redshift: g_grav = sqrt(1 - rs / r)
    float gGrav = sqrt(clamp(1.0 - rs / r, 0.01, 1.0));
    float totalShift = dopplerFactor * gGrav;

    // Relativistic Beaming: Bolometric Flux scales as g^4
    float beaming = clamp(pow(totalShift, 4.0), 0.02, 18.0);

    // Observed blackbody color at shifted temperature
    float observedTemp = localTemp * totalShift;
    vec3 emitCol = temperatureToColor(observedTemp);

    // Procedural Differential Keplerian Sheared Turbulence (Seamless: no branch cut)
    vec3 refX = normalize(abs(spinNorm.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0));
    vec3 refY = cross(spinNorm, refX);
    float phi = atan(dot(hitPos, refY), dot(hitPos, refX));

    float omegaKepler = pow(rIn / r, 1.5);
    float phiSheared = phi - omegaKepler * (uSimTime * 0.6 + 4.5 * log(rNorm));

    // Continuous 2D periodic coordinates eliminate any seam at phi = +/- PI
    vec3 noiseCoord = vec3(cos(phiSheared) * 2.8, sin(phiSheared) * 2.8, rNorm * 0.65);
    float turbulence = 0.55 + 0.45 * fbmDisk(noiseCoord);

    // Harmonic multi-arm logarithmic spiral density waves
    float spiral1 = cos(2.0 * phiSheared - 3.0 * log(rNorm));
    float spiral2 = cos(4.0 * phiSheared - 4.5 * log(rNorm));
    float spiralArm = 0.76 + 0.18 * spiral1 + 0.06 * spiral2;

    // Sharp ISCO inner edge truncation and gentle outer rim fade
    float innerCut = smoothstep(rIn * 0.98, rIn * 1.03, r);
    float outerCut = smoothstep(rOut * 1.01, rOut * 0.70, r);
    float density = turbulence * spiralArm * innerCut * outerCut;

    // Radiative transfer optical depth
    float opacityCoeff = 2.4 / max(rs, 1e-4);
    float opticalDepth = density * dsPath * opacityCoeff;
    float alphaHit = clamp(1.0 - exp(-opticalDepth), 0.0, 0.95);

    vec3 hitRadiance = emitCol * beaming * (0.80 + 0.40 * turbulence) * 2.2;

    diskRadiance += (1.0 - diskAlpha) * hitRadiance * alphaHit;
    diskAlpha += (1.0 - diskAlpha) * alphaHit;
}

void main() {
    vec3 camRel = uCameraPos - uBhCenter;
    float camDist = length(camRel);
    vec3 rayDir = normalize(RayDir);

    float rs = max(uSchwRadius, 1e-5);
    float rHorizon = max(uHorizonRadius, rs * 0.5);
    float rBound = max(uBoundRadius, rs * 6.0);
    float rIn = max(uDiskInnerRadius, rHorizon * 1.02);
    float rOut = max(uDiskOuterRadius, rIn * 1.5);

    vec3 spinNorm = length(uSpinAxis) > 0.01 ? normalize(uSpinAxis) : vec3(0.0, 1.0, 0.0);

    // Bounding sphere intersection test
    float bProj = dot(camRel, rayDir);
    float cVal = dot(camRel, camRel) - rBound * rBound;
    float disc = bProj * bProj - cVal;

    vec3 rayPos;
    if (camDist >= rBound) {
        if (bProj > 0.0 || disc < 0.0) {
            // Ray passes outside the black hole influence zone: pass through unbent skybox
            discard;
        }
        float tEnter = -bProj - sqrt(disc);
        rayPos = camRel + rayDir * max(0.0, tEnter);
    } else {
        rayPos = camRel;
    }

    vec3 diskRadiance = vec3(0.0);
    float diskAlpha = 0.0;
    bool rayCaptured = false;
    float minRayDist = length(rayPos);
    float accumulatedAngle = 0.0;

    int maxSteps = (uVisualMode >= 2) ? 144 : (uVisualMode == 1 ? 112 : 72);

    vec3 curPos = rayPos;
    vec3 curDir = rayDir;

    for (int step = 0; step < maxSteps; ++step) {
        float r = length(curPos);
        minRayDist = min(minRayDist, r);

        // 1. Relativistic Capture: Inward ray inside photon sphere or horizon cannot escape
        if (r <= rHorizon * 1.01 || (r <= uPhotonRadius * 1.01 && dot(curPos, curDir) < 0.0)) {
            rayCaptured = true;
            break;
        }

        // 2. Escape Condition
        if (r > rBound && dot(curPos, curDir) > 0.0) {
            break;
        }

        // 3. Adaptive Step Size
        float dt;
        if (r < rs * 3.0) {
            dt = clamp(0.038 * r * sqrt(r / rs), 0.005 * rs, 0.12 * rs);
        } else if (r < rs * 8.0) {
            dt = clamp(0.065 * r, 0.04 * rs, 0.30 * rs);
        } else {
            dt = clamp(0.12 * r, 0.18 * rs, 0.08 * rBound);
        }

        // 4. 4th-Order Runge-Kutta (RK4) Geodesic Integrator
        vec3 k1_v = curDir;
        vec3 k1_a = computeGeodesicAcceleration(curPos, curDir, rs, uSpinParam, spinNorm);

        vec3 p2 = curPos + 0.5 * dt * k1_v;
        vec3 v2 = normalize(curDir + 0.5 * dt * k1_a);
        vec3 k2_v = v2;
        vec3 k2_a = computeGeodesicAcceleration(p2, v2, rs, uSpinParam, spinNorm);

        vec3 p3 = curPos + 0.5 * dt * k2_v;
        vec3 v3 = normalize(curDir + 0.5 * dt * k2_a);
        vec3 k3_v = v3;
        vec3 k3_a = computeGeodesicAcceleration(p3, v3, rs, uSpinParam, spinNorm);

        vec3 p4 = curPos + dt * k3_v;
        vec3 v4 = normalize(curDir + dt * k3_a);
        vec3 k4_v = v4;
        vec3 k4_a = computeGeodesicAcceleration(p4, v4, rs, uSpinParam, spinNorm);

        vec3 nextPos = curPos + (dt / 6.0) * (k1_v + 2.0 * k2_v + 2.0 * k3_v + k4_v);
        vec3 nextDir = normalize(curDir + (dt / 6.0) * (k1_a + 2.0 * k2_a + 2.0 * k3_a + k4_a));

        accumulatedAngle += acos(clamp(dot(curDir, nextDir), -1.0, 1.0));

        // 5. Analytic Equatorial Accretion Disk Crossing
        // Accurately catches both the front disk and the lensed upper and lower arches
        if (uHasDisk && diskAlpha < 0.98) {
            float z0 = dot(curPos, spinNorm);
            float z1 = dot(nextPos, spinNorm);

            // Did the ray cross the equatorial plane z = 0 during this step?
            if (z0 * z1 <= 0.0 && z0 != z1) {
                float tHit = clamp(z0 / (z0 - z1), 0.0, 1.0);
                vec3 hitPos = mix(curPos, nextPos, tHit);
                vec3 avgDir = normalize(mix(curDir, nextDir, tHit));

                sampleDiskAtPoint(hitPos, avgDir, rs, rIn, rOut, spinNorm, diskRadiance, diskAlpha);
            }
        }

        curPos = nextPos;
        curDir = nextDir;
    }

    vec3 finalColor = vec3(0.0);
    vec3 brightColor = vec3(0.0);

    if (rayCaptured) {
        // Pure black shadow: only foreground disk emission is visible in front of horizon
        finalColor = diskRadiance;
        brightColor = max(vec3(0.0), diskRadiance - vec3(1.10)) * 0.45;
    } else {
        // Deflected Background Light from Stars / Skybox
        vec3 skyColor = sampleSkyboxColor(curDir);

        // Razor-thin exponential relativistic photon ring highlight
        // Concentrated strictly at critical boundary for highly deflected rays
        if (minRayDist < rs * 1.62 && minRayDist > rHorizon && accumulatedAngle > 1.9 * PI) {
            float turns = clamp((accumulatedAngle - 1.9 * PI) / (1.2 * PI), 0.0, 1.0);
            float ringPeak = pow(turns, 3.0) * smoothstep(rs * 1.62, rs * 1.50, minRayDist);
            vec3 photonHighlight = vec3(1.0, 0.98, 0.94) * ringPeak * (uHasDisk ? 3.0 : 1.5);
            skyColor += photonHighlight;
        }

        // Composite accretion disk emission over lensed background sky and photon ring
        finalColor = diskRadiance + (1.0 - diskAlpha) * skyColor;

        // HDR Bright-Pass extraction for bloom
        vec3 excessLum = max(vec3(0.0), finalColor - vec3(1.10));
        brightColor = excessLum * 0.45;
    }

    FragColor = vec4(finalColor, 1.0);
    BrightColor = vec4(brightColor, 1.0);
}
