#include "data/UnitConverter.hpp"
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace AstroGenesis {

double UnitConverter::calendarToJulianDate(int year, int month, int day, int hour, int minute, double second) {
    int y = year;
    int m = month;
    if (m <= 2) {
        y -= 1;
        m += 12;
    }

    int a = y / 100;
    int b = 2 - a + (a / 4);

    double dayFraction = (hour + (minute / 60.0) + (second / 3600.0)) / 24.0;
    double jd = std::floor(365.25 * (y + 4716)) + std::floor(30.6001 * (m + 1)) + day + dayFraction + b - 1524.5;
    return jd;
}

std::string UnitConverter::julianDateToUtcString(double jd) {
    double z = std::floor(jd + 0.5);
    double f = (jd + 0.5) - z;

    long long a = (long long)z;
    if (z >= 2299161) {
        long long alpha = (long long)std::floor((z - 1867216.25) / 36524.25);
        a = (long long)(z + 1 + alpha - (alpha / 4));
    }

    long long b = a + 1524;
    long long c = (long long)std::floor((b - 122.1) / 365.25);
    long long d = (long long)std::floor(365.25 * c);
    long long e = (long long)std::floor((b - d) / 30.6001);

    int day = (int)(b - d - (long long)std::floor(30.6001 * e));
    int month = (int)((e < 14) ? (e - 1) : (e - 13));
    int year = (int)((month > 2) ? (c - 4716) : (c - 4715));

    double totalSeconds = f * 86400.0;
    int hour = (int)(totalSeconds / 3600.0);
    totalSeconds -= hour * 3600.0;
    int minute = (int)(totalSeconds / 60.0);
    double second = totalSeconds - minute * 60.0;

    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%04.1f UTC", year, month, day, hour, minute, second);
    return std::string(buf);
}

double UnitConverter::iso8601ToJulianDate(const std::string& isoStr) {
    int y = 2000, m = 1, d = 1, hr = 0, min = 0;
    double sec = 0.0;
    if (sscanf(isoStr.c_str(), "%d-%d-%d %d:%d:%lf", &y, &m, &d, &hr, &min, &sec) >= 3 ||
        sscanf(isoStr.c_str(), "%d-%d-%dT%d:%d:%lf", &y, &m, &d, &hr, &min, &sec) >= 3) {
        return calendarToJulianDate(y, m, d, hr, min, sec);
    }
    return J2000_JD;
}

std::string UnitConverter::formatDistance(double meters) {
    char buf[64];
    double au = meters * METERS_TO_AU;
    if (au >= 0.01) {
        snprintf(buf, sizeof(buf), "%.3f AU (%.2fM km)", au, (meters * 1e-9));
    } else {
        snprintf(buf, sizeof(buf), "%'.1f km", meters / 1000.0);
    }
    return std::string(buf);
}

std::string UnitConverter::formatMass(double kg) {
    char buf[64];
    if (kg >= 1.0e30) {
        snprintf(buf, sizeof(buf), "%.3f M☉ (%.2e kg)", kg / SOLAR_MASS_KG, kg);
    } else if (kg >= 1.0e26) {
        snprintf(buf, sizeof(buf), "%.2f M_Jup (%.3e kg)", kg / JUPITER_MASS_KG, kg);
    } else if (kg >= 1.0e23) {
        snprintf(buf, sizeof(buf), "%.3f M⊕ (%.3e kg)", kg / EARTH_MASS_KG, kg);
    } else {
        snprintf(buf, sizeof(buf), "%.2e kg", kg);
    }
    return std::string(buf);
}

std::string UnitConverter::formatVelocity(double metersPerSec) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f km/s", metersPerSec / 1000.0);
    return std::string(buf);
}

std::string UnitConverter::formatPeriod(double seconds) {
    char buf[64];
    double days = seconds / SEC_PER_DAY;
    if (days >= 365.25 * 1.5) {
        snprintf(buf, sizeof(buf), "%.2f years", days / 365.256);
    } else if (days >= 1.0) {
        snprintf(buf, sizeof(buf), "%.2f days", days);
    } else {
        double hours = seconds / 3600.0;
        snprintf(buf, sizeof(buf), "%.2f hours", hours);
    }
    return std::string(buf);
}

std::string UnitConverter::formatAngleDeg(double degrees) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f°", degrees);
    return std::string(buf);
}

std::string UnitConverter::formatPressure(double pascals) {
    char buf[64];
    if (pascals >= 1000.0) {
        snprintf(buf, sizeof(buf), "%.1f kPa", pascals / 1000.0);
    } else if (pascals > 0.0) {
        snprintf(buf, sizeof(buf), "%.2e Pa", pascals);
    } else {
        snprintf(buf, sizeof(buf), "N/A");
    }
    return std::string(buf);
}

std::string UnitConverter::formatDensity(double kgM3) {
    char buf[64];
    if (kgM3 > 0.0) {
        snprintf(buf, sizeof(buf), "%'.1f kg/m³", kgM3);
    } else {
        snprintf(buf, sizeof(buf), "N/A");
    }
    return std::string(buf);
}

void UnitConverter::keplerianToCartesian(double aMeters,
                                        double eccentricity,
                                        double inclinationDeg,
                                        double longAscNodeDeg,
                                        double argPeriapsisDeg,
                                        double meanAnomalyDeg,
                                        double centralMassKg,
                                        double bodyMassKg,
                                        double& outPosX, double& outPosY, double& outPosZ,
                                        double& outVelX, double& outVelY, double& outVelZ) {
    if (aMeters <= 0.0) aMeters = 1.0 * AU_TO_METERS;
    double e = std::clamp(eccentricity, 0.0, 0.999);
    double iRad = degToRad(inclinationDeg);
    double omegaNodeRad = degToRad(longAscNodeDeg);
    double argPeriRad = degToRad(argPeriapsisDeg);
    double mRad = degToRad(meanAnomalyDeg);

    // 1. Solve Kepler's Equation for Eccentric Anomaly E: M = E - e*sin(E)
    double E = mRad;
    for (int iter = 0; iter < 15; ++iter) {
        double f = E - e * std::sin(E) - mRad;
        double fPrime = 1.0 - e * std::cos(E);
        double delta = f / fPrime;
        E -= delta;
        if (std::abs(delta) < 1e-12) break;
    }

    // 2. Compute True Anomaly nu
    double sinNu = (std::sqrt(1.0 - e * e) * std::sin(E)) / (1.0 - e * std::cos(E));
    double cosNu = (std::cos(E) - e) / (1.0 - e * std::cos(E));
    double nu = std::atan2(sinNu, cosNu);

    // 3. Radial distance r and speed in perifocal coordinate frame
    double r = (aMeters * (1.0 - e * e)) / (1.0 + e * std::cos(nu));
    double mu = G_CONST * (centralMassKg + bodyMassKg);
    double p = aMeters * (1.0 - e * e);
    double h = std::sqrt(std::max(1.0, mu * p));

    double x_peri = r * std::cos(nu);
    double y_peri = r * std::sin(nu);

    double vx_peri = - (mu / h) * std::sin(nu);
    double vy_peri = (mu / h) * (e + std::cos(nu));

    // 4. Standard 3D Euler Transformation to Ecliptic Frame: R_z(Omega) * R_x(i) * R_z(omega)
    double cosNode = std::cos(omegaNodeRad);
    double sinNode = std::sin(omegaNodeRad);
    double cosInc = std::cos(iRad);
    double sinInc = std::sin(iRad);
    double cosArg = std::cos(argPeriRad);
    double sinArg = std::sin(argPeriRad);

    // Unit vectors in ecliptic plane:
    double P_x = cosNode * cosArg - sinNode * sinArg * cosInc;
    double P_y = sinNode * cosArg + cosNode * sinArg * cosInc;
    double P_z = sinArg * sinInc;

    double Q_x = -cosNode * sinArg - sinNode * cosArg * cosInc;
    double Q_y = -sinNode * sinArg + cosNode * cosArg * cosInc;
    double Q_z = cosArg * sinInc;

    double posEcl_x = x_peri * P_x + y_peri * Q_x;
    double posEcl_y = x_peri * P_y + y_peri * Q_y;
    double posEcl_z = x_peri * P_z + y_peri * Q_z;

    double velEcl_x = vx_peri * P_x + vy_peri * Q_x;
    double velEcl_y = vx_peri * P_y + vy_peri * Q_y;
    double velEcl_z = vx_peri * P_z + vy_peri * Q_z;

    // 5. Map to AstroGenesis Coordinate System:
    // AstroGenesis: X = Ecliptic X, Y = Ecliptic Z (UP / Normal), Z = Ecliptic Y (In-Plane Depth)
    outPosX = posEcl_x;
    outPosY = posEcl_z;
    outPosZ = posEcl_y;

    outVelX = velEcl_x;
    outVelY = velEcl_z;
    outVelZ = velEcl_y;
}

} // namespace AstroGenesis
