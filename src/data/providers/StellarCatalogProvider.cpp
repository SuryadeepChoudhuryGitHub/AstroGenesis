#include "data/providers/StellarCatalogProvider.hpp"
#include "data/UnitConverter.hpp"
#include "simulation/CelestialBody.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace AstroGenesis {

StellarCatalogProvider::StellarCatalogProvider(HttpClient& httpClient) : m_http(httpClient) {
    initializeCuratedCatalog();
}

void StellarCatalogProvider::initializeCuratedCatalog() {
    // ── 1. Sirius System (Binary Star System: Sirius A & Sirius B) ──
    CuratedStarEntry siriusA;
    siriusA.slug = "sirius_a";
    siriusA.name = "Sirius A";
    siriusA.systemName = "Sirius System";
    siriusA.spectralType = "A1V";
    siriusA.classification = "Star (Main Sequence)";
    siriusA.massMsun = 2.063;
    siriusA.radiusRsun = 1.711;
    siriusA.teffK = 9940.0;
    siriusA.lumLsun = 25.4;
    siriusA.distanceLy = 8.60;
    siriusA.vMag = -1.46;
    siriusA.mainAlias = "Alpha Canis Majoris A";
    siriusA.crossIdents = "HD 48915, HIP 32349, HR 2491, BD-16 1591, ADS 5423 A";
    siriusA.color = glm::vec3(0.97f, 0.98f, 1.0f);
    siriusA.isBinaryComponent = true;
    siriusA.binarySemiMajorAU = 7.49;
    siriusA.binaryPeriodYr = 50.13;
    siriusA.binaryEcc = 0.5914;
    double siriusDistM = 7.49 * UnitConverter::AU_TO_METERS;
    double mTotal = (2.063 + 1.018) * UnitConverter::SOLAR_MASS_KG;
    double rA = siriusDistM * (1.018 / (2.063 + 1.018));
    double vRel = std::sqrt(UnitConverter::G_CONST * mTotal / siriusDistM);
    double vA = vRel * (1.018 / (2.063 + 1.018));
    siriusA.barycentricPosM = glm::dvec3(-rA, 0.0, 0.0);
    siriusA.barycentricVelMps = glm::dvec3(0.0, 0.0, -vA);
    m_curatedStars[siriusA.slug] = siriusA;

    CuratedStarEntry siriusB;
    siriusB.slug = "sirius_b";
    siriusB.name = "Sirius B";
    siriusB.systemName = "Sirius System";
    siriusB.spectralType = "DA1.9";
    siriusB.classification = "Star (White Dwarf)";
    siriusB.massMsun = 1.018;
    siriusB.radiusRsun = 0.0084; // 5,846 km (~0.92 Earth radii)
    siriusB.teffK = 25200.0;
    siriusB.lumLsun = 0.056;
    siriusB.distanceLy = 8.60;
    siriusB.vMag = 8.44;
    siriusB.mainAlias = "Alpha Canis Majoris B";
    siriusB.crossIdents = "HD 48915B, HIP 32349B, EGGR 49, WD 0642-166, ADS 5423 B";
    siriusB.color = glm::vec3(0.72f, 0.82f, 1.0f); // Intense hot blue-white
    siriusB.isBinaryComponent = true;
    siriusB.binarySemiMajorAU = 7.49;
    siriusB.binaryPeriodYr = 50.13;
    siriusB.binaryEcc = 0.5914;
    double rB = siriusDistM * (2.063 / (2.063 + 1.018));
    double vB = vRel * (2.063 / (2.063 + 1.018));
    siriusB.barycentricPosM = glm::dvec3(rB, 0.0, 0.0);
    siriusB.barycentricVelMps = glm::dvec3(0.0, 0.0, vB);
    m_curatedStars[siriusB.slug] = siriusB;

    CuratedSystemEntry siriusSys;
    siriusSys.systemName = "Sirius System";
    siriusSys.type = "Binary Star System";
    siriusSys.description = "Visual binary system consisting of Sirius A (A1V) and Sirius B (DA1.9 White Dwarf), orbiting their mutual barycenter every 50.1 years.";
    siriusSys.componentSlugs = { "sirius_a", "sirius_b" };
    m_curatedSystems["sirius_system"] = siriusSys;

    // ── 2. Rigel (Blue Supergiant Single-Star System) ──
    CuratedStarEntry rigel;
    rigel.slug = "rigel";
    rigel.name = "Rigel";
    rigel.systemName = "Rigel System";
    rigel.spectralType = "B8Ia";
    rigel.classification = "Star (Supergiant)";
    rigel.massMsun = 21.0;
    rigel.radiusRsun = 78.9;
    rigel.teffK = 12100.0;
    rigel.lumLsun = 120000.0;
    rigel.distanceLy = 860.0;
    rigel.vMag = 0.13;
    rigel.mainAlias = "Beta Orionis";
    rigel.crossIdents = "HD 34085, HIP 24436, HR 1713, SAO 131907";
    rigel.color = glm::vec3(0.70f, 0.82f, 1.0f);
    m_curatedStars[rigel.slug] = rigel;

    CuratedSystemEntry rigelSys;
    rigelSys.systemName = "Rigel System";
    rigelSys.type = "Single Star System";
    rigelSys.description = "Luminous Blue Supergiant in the constellation Orion, with 120,000x solar luminosity and 78.9 solar radii.";
    rigelSys.componentSlugs = { "rigel" };
    m_curatedSystems["rigel_system"] = rigelSys;

    // ── 3. Betelgeuse (Red Supergiant) ──
    CuratedStarEntry betel;
    betel.slug = "betelgeuse";
    betel.name = "Betelgeuse";
    betel.systemName = "Betelgeuse System";
    betel.spectralType = "M1-M2Ia-Iab";
    betel.classification = "Star (Supergiant)";
    betel.massMsun = 16.5;
    betel.radiusRsun = 764.0;
    betel.teffK = 3600.0;
    betel.lumLsun = 126000.0;
    betel.distanceLy = 548.0;
    betel.vMag = 0.42;
    betel.mainAlias = "Alpha Orionis";
    betel.crossIdents = "HD 39801, HIP 27989, HR 2061";
    betel.color = glm::vec3(1.0f, 0.45f, 0.18f);
    m_curatedStars[betel.slug] = betel;

    CuratedSystemEntry betelSys;
    betelSys.systemName = "Betelgeuse System";
    betelSys.type = "Single Star System";
    betelSys.description = "Massive pulsating Red Supergiant nearing core collapse supernova stage, spanning over 3.5 AU in radius.";
    betelSys.componentSlugs = { "betelgeuse" };
    m_curatedSystems["betelgeuse_system"] = betelSys;

    // ── 4. Vega (A0V Main Sequence Standard) ──
    CuratedStarEntry vega;
    vega.slug = "vega";
    vega.name = "Vega";
    vega.systemName = "Vega System";
    vega.spectralType = "A0V";
    vega.classification = "Star (Main Sequence)";
    vega.massMsun = 2.135;
    vega.radiusRsun = 2.362;
    vega.teffK = 9602.0;
    vega.lumLsun = 40.12;
    vega.distanceLy = 25.04;
    vega.vMag = 0.03;
    vega.mainAlias = "Alpha Lyrae";
    vega.crossIdents = "HD 172167, HIP 91262, HR 7001";
    vega.color = glm::vec3(0.96f, 0.98f, 1.0f);
    m_curatedStars[vega.slug] = vega;

    CuratedSystemEntry vegaSys;
    vegaSys.systemName = "Vega System";
    vegaSys.type = "Single Star System";
    vegaSys.description = "A-type main sequence benchmark star with a circumstellar debris disk, serving as zero-point photometric calibration.";
    vegaSys.componentSlugs = { "vega" };
    m_curatedSystems["vega_system"] = vegaSys;

    // ── 5. Polaris (North Star Yellow Supergiant) ──
    CuratedStarEntry polaris;
    polaris.slug = "polaris";
    polaris.name = "Polaris";
    polaris.systemName = "Polaris System";
    polaris.spectralType = "F7Ib";
    polaris.classification = "Star (Supergiant)";
    polaris.massMsun = 5.4;
    polaris.radiusRsun = 37.5;
    polaris.teffK = 6015.0;
    polaris.lumLsun = 1260.0;
    polaris.distanceLy = 433.0;
    polaris.vMag = 1.98;
    polaris.mainAlias = "Alpha Ursae Minoris";
    polaris.crossIdents = "HD 8890, HIP 11767, HR 424";
    polaris.color = glm::vec3(1.0f, 0.95f, 0.85f);
    m_curatedStars[polaris.slug] = polaris;

    CuratedSystemEntry polarisSys;
    polarisSys.systemName = "Polaris System";
    polarisSys.type = "Single Star System";
    polarisSys.description = "Classical Cepheid variable Yellow Supergiant aligning currently with Earth's Northern celestial rotational pole.";
    polarisSys.componentSlugs = { "polaris" };
    m_curatedSystems["polaris_system"] = polarisSys;

    // ── 6. Proxima Centauri (Nearest Star Red Dwarf) ──
    CuratedStarEntry proxima;
    proxima.slug = "proxima_centauri";
    proxima.name = "Proxima Centauri";
    proxima.systemName = "Proxima Centauri System";
    proxima.spectralType = "M5.5Ve";
    proxima.classification = "Star (Red Dwarf)";
    proxima.massMsun = 0.1221;
    proxima.radiusRsun = 0.1542;
    proxima.teffK = 3042.0;
    proxima.lumLsun = 0.0017;
    proxima.distanceLy = 4.246;
    proxima.vMag = 11.13;
    proxima.mainAlias = "Alpha Centauri C";
    proxima.crossIdents = "HIP 70890, V645 Cen, GJ 551";
    proxima.color = glm::vec3(1.0f, 0.38f, 0.15f);
    m_curatedStars[proxima.slug] = proxima;

    CuratedSystemEntry proximaSys;
    proximaSys.systemName = "Proxima Centauri System";
    proximaSys.type = "Single Star System";
    proximaSys.description = "Lowest-mass flare star and closest known stellar neighbor to the Solar System at 4.246 light years.";
    proximaSys.componentSlugs = { "proxima_centauri" };
    m_curatedSystems["proxima_centauri_system"] = proximaSys;

    // ── 7. Alpha Centauri AB (Binary System) ──
    CuratedStarEntry cenA;
    cenA.slug = "alpha_centauri_a";
    cenA.name = "Alpha Centauri A";
    cenA.systemName = "Alpha Centauri System";
    cenA.spectralType = "G2V";
    cenA.classification = "Star (Main Sequence)";
    cenA.massMsun = 1.100;
    cenA.radiusRsun = 1.223;
    cenA.teffK = 5790.0;
    cenA.lumLsun = 1.519;
    cenA.distanceLy = 4.37;
    cenA.vMag = -0.01;
    cenA.mainAlias = "Rigil Kentaurus";
    cenA.crossIdents = "HD 128620, HIP 71683, HR 5459";
    cenA.color = glm::vec3(1.0f, 0.88f, 0.35f);
    cenA.isBinaryComponent = true;
    cenA.binarySemiMajorAU = 23.4;
    cenA.binaryPeriodYr = 79.91;
    cenA.binaryEcc = 0.518;
    double cenDistM = 23.4 * UnitConverter::AU_TO_METERS;
    double cenTotalM = (1.100 + 0.907) * UnitConverter::SOLAR_MASS_KG;
    double cenRA = cenDistM * (0.907 / (1.100 + 0.907));
    double cenVRel = std::sqrt(UnitConverter::G_CONST * cenTotalM / cenDistM);
    double cenVA = cenVRel * (0.907 / (1.100 + 0.907));
    cenA.barycentricPosM = glm::dvec3(-cenRA, 0.0, 0.0);
    cenA.barycentricVelMps = glm::dvec3(0.0, 0.0, -cenVA);
    m_curatedStars[cenA.slug] = cenA;

    CuratedStarEntry cenB;
    cenB.slug = "alpha_centauri_b";
    cenB.name = "Alpha Centauri B";
    cenB.systemName = "Alpha Centauri System";
    cenB.spectralType = "K1V";
    cenB.classification = "Star (Main Sequence)";
    cenB.massMsun = 0.907;
    cenB.radiusRsun = 0.863;
    cenB.teffK = 5260.0;
    cenB.lumLsun = 0.500;
    cenB.distanceLy = 4.37;
    cenB.vMag = 1.33;
    cenB.mainAlias = "Toliman";
    cenB.crossIdents = "HD 128621, HIP 71681, HR 5460";
    cenB.color = glm::vec3(1.0f, 0.65f, 0.20f);
    cenB.isBinaryComponent = true;
    cenB.binarySemiMajorAU = 23.4;
    cenB.binaryPeriodYr = 79.91;
    cenB.binaryEcc = 0.518;
    double cenRB = cenDistM * (1.100 / (1.100 + 0.907));
    double cenVB = cenVRel * (1.100 / (1.100 + 0.907));
    cenB.barycentricPosM = glm::dvec3(cenRB, 0.0, 0.0);
    cenB.barycentricVelMps = glm::dvec3(0.0, 0.0, cenVB);
    m_curatedStars[cenB.slug] = cenB;

    CuratedSystemEntry cenSys;
    cenSys.systemName = "Alpha Centauri System";
    cenSys.type = "Binary Star System";
    cenSys.description = "Primary binary star system composed of Sun-like star Alpha Centauri A and K-dwarf Alpha Centauri B, orbiting with 23.4 AU semi-major axis.";
    cenSys.componentSlugs = { "alpha_centauri_a", "alpha_centauri_b" };
    m_curatedSystems["alpha_centauri_system"] = cenSys;

    // ── 8. Deneb (White Supergiant) ──
    CuratedStarEntry deneb;
    deneb.slug = "deneb";
    deneb.name = "Deneb";
    deneb.systemName = "Deneb System";
    deneb.spectralType = "A2Ia";
    deneb.classification = "Star (Supergiant)";
    deneb.massMsun = 19.0;
    deneb.radiusRsun = 203.0;
    deneb.teffK = 8525.0;
    deneb.lumLsun = 196000.0;
    deneb.distanceLy = 2615.0;
    deneb.vMag = 1.25;
    deneb.mainAlias = "Alpha Cygni";
    deneb.crossIdents = "HD 197345, HIP 102098, HR 7924";
    deneb.color = glm::vec3(0.92f, 0.95f, 1.0f);
    m_curatedStars[deneb.slug] = deneb;

    CuratedSystemEntry denebSys;
    denebSys.systemName = "Deneb System";
    denebSys.type = "Single Star System";
    denebSys.description = "One of the most intrinsically luminous stars known in the local Milky Way, anchoring the Summer Triangle.";
    denebSys.componentSlugs = { "deneb" };
    m_curatedSystems["deneb_system"] = denebSys;

    // ── 9. Arcturus (Red Giant Benchmark) ──
    CuratedStarEntry arcturus;
    arcturus.slug = "arcturus";
    arcturus.name = "Arcturus";
    arcturus.systemName = "Arcturus System";
    arcturus.spectralType = "K0III";
    arcturus.classification = "Star (Giant)";
    arcturus.massMsun = 1.08;
    arcturus.radiusRsun = 25.4;
    arcturus.teffK = 4286.0;
    arcturus.lumLsun = 170.0;
    arcturus.distanceLy = 36.7;
    arcturus.vMag = -0.05;
    arcturus.mainAlias = "Alpha Bootis";
    arcturus.crossIdents = "HD 124897, HIP 69673, HR 5340";
    arcturus.color = glm::vec3(1.0f, 0.62f, 0.22f);
    m_curatedStars[arcturus.slug] = arcturus;

    CuratedSystemEntry arcturusSys;
    arcturusSys.systemName = "Arcturus System";
    arcturusSys.type = "Single Star System";
    arcturusSys.description = "Evolved Red Giant star in Boötes exhibiting high proper motion and low metallicity characteristic of the Galactic thick disk.";
    arcturusSys.componentSlugs = { "arcturus" };
    m_curatedSystems["arcturus_system"] = arcturusSys;

    // ── 10. Aldebaran (Orange Giant) ──
    CuratedStarEntry aldebaran;
    aldebaran.slug = "aldebaran";
    aldebaran.name = "Aldebaran";
    aldebaran.systemName = "Aldebaran System";
    aldebaran.spectralType = "K5III";
    aldebaran.classification = "Star (Giant)";
    aldebaran.massMsun = 1.16;
    aldebaran.radiusRsun = 44.1;
    aldebaran.teffK = 3900.0;
    aldebaran.lumLsun = 439.0;
    aldebaran.distanceLy = 65.3;
    aldebaran.vMag = 0.85;
    aldebaran.mainAlias = "Alpha Tauri";
    aldebaran.crossIdents = "HD 29139, HIP 21421, HR 1457";
    aldebaran.color = glm::vec3(1.0f, 0.52f, 0.18f);
    m_curatedStars[aldebaran.slug] = aldebaran;

    CuratedSystemEntry aldebaranSys;
    aldebaranSys.systemName = "Aldebaran System";
    aldebaranSys.type = "Single Star System";
    aldebaranSys.description = "Giant star in the line of sight to the Hyades cluster, shining brightly in the constellation Taurus.";
    aldebaranSys.componentSlugs = { "aldebaran" };
    m_curatedSystems["aldebaran_system"] = aldebaranSys;

    // ── 11. Cygnus X-1 System (Black Hole + Blue Supergiant Binary) ──
    CuratedStarEntry cygBH;
    cygBH.slug = "cygnus_x1_bh";
    cygBH.name = "Cygnus X-1 Singularity";
    cygBH.systemName = "Cygnus X-1 System";
    cygBH.spectralType = "Stellar-Mass Singularity";
    cygBH.classification = "Black Hole";
    cygBH.massMsun = 21.2;
    cygBH.radiusRsun = 29530.0 / UnitConverter::SOLAR_RADIUS_M; // Rs = 62.6 km
    cygBH.teffK = 1e-8;
    cygBH.lumLsun = 0.0;
    cygBH.distanceLy = 7200.0;
    cygBH.vMag = 18.0;
    cygBH.mainAlias = "Cyg X-1 Relativistic Singularity";
    cygBH.crossIdents = "V1357 Cyg Primary, BH 1956+350";
    cygBH.color = glm::vec3(0.55f, 0.15f, 0.85f);
    cygBH.isBinaryComponent = true;
    cygBH.binarySemiMajorAU = 0.24;
    cygBH.binaryPeriodYr = 0.0153; // 5.6 days
    double cygDistM = 0.24 * UnitConverter::AU_TO_METERS;
    double cygTotM = (21.2 + 40.6) * UnitConverter::SOLAR_MASS_KG;
    double cygRBH = cygDistM * (40.6 / (21.2 + 40.6));
    double cygVRel = std::sqrt(UnitConverter::G_CONST * cygTotM / cygDistM);
    double cygVBH = cygVRel * (40.6 / (21.2 + 40.6));
    cygBH.barycentricPosM = glm::dvec3(-cygRBH, 0.0, 0.0);
    cygBH.barycentricVelMps = glm::dvec3(0.0, 0.0, -cygVBH);
    m_curatedStars[cygBH.slug] = cygBH;

    CuratedStarEntry cygStar;
    cygStar.slug = "cygnus_x1_hde226868";
    cygStar.name = "HDE 226868";
    cygStar.systemName = "Cygnus X-1 System";
    cygStar.spectralType = "O9.7Iab";
    cygStar.classification = "Star (Supergiant)";
    cygStar.massMsun = 40.6;
    cygStar.radiusRsun = 22.3;
    cygStar.teffK = 31000.0;
    cygStar.lumLsun = 260000.0;
    cygStar.distanceLy = 7200.0;
    cygStar.vMag = 8.95;
    cygStar.mainAlias = "V1357 Cyg Secondary";
    cygStar.crossIdents = "HD 226868, HIP 98298, BD+34 3815";
    cygStar.color = glm::vec3(0.40f, 0.70f, 1.0f);
    cygStar.isBinaryComponent = true;
    cygStar.binarySemiMajorAU = 0.24;
    cygStar.binaryPeriodYr = 0.0153;
    double cygRStar = cygDistM * (21.2 / (21.2 + 40.6));
    double cygVStar = cygVRel * (21.2 / (21.2 + 40.6));
    cygStar.barycentricPosM = glm::dvec3(cygRStar, 0.0, 0.0);
    cygStar.barycentricVelMps = glm::dvec3(0.0, 0.0, cygVStar);
    m_curatedStars[cygStar.slug] = cygStar;

    CuratedSystemEntry cygSys;
    cygSys.systemName = "Cygnus X-1 System";
    cygSys.type = "Binary Star System";
    cygSys.description = "First confirmed black hole system in history, composed of a 21.2 M☉ black hole fed by Roche lobe accretion from O9.7Iab companion HDE 226868.";
    cygSys.componentSlugs = { "cygnus_x1_bh", "cygnus_x1_hde226868" };
    m_curatedSystems["cygnus_x1_system"] = cygSys;

    // ── Build alias lookups ──
    for (const auto& kv : m_curatedStars) {
        const auto& s = kv.second;
        std::string lowName = s.name;
        std::transform(lowName.begin(), lowName.end(), lowName.begin(), ::tolower);
        m_aliasToSlug[lowName] = s.slug;

        std::string lowSlug = s.slug;
        std::transform(lowSlug.begin(), lowSlug.end(), lowSlug.begin(), ::tolower);
        m_aliasToSlug[lowSlug] = s.slug;

        std::string lowAlias = s.mainAlias;
        std::transform(lowAlias.begin(), lowAlias.end(), lowAlias.begin(), ::tolower);
        m_aliasToSlug[lowAlias] = s.slug;

        // Extract HD, HIP tokens
        std::stringstream ss(s.crossIdents);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            size_t start = tok.find_first_not_of(" \t");
            size_t end = tok.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                std::string clean = tok.substr(start, end - start + 1);
                std::transform(clean.begin(), clean.end(), clean.begin(), ::tolower);
                m_aliasToSlug[clean] = s.slug;
            }
        }
    }

    for (const auto& kv : m_curatedSystems) {
        std::string low = kv.second.systemName;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
        m_aliasToSlug[low] = kv.first;
    }
}

bool StellarCatalogProvider::isLikelyStellarQuery(const std::string& query) {
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    if (q.find("sirius") != std::string::npos || q.find("rigel") != std::string::npos ||
        q.find("betelgeuse") != std::string::npos || q.find("vega") != std::string::npos ||
        q.find("polaris") != std::string::npos || q.find("proxima") != std::string::npos ||
        q.find("alpha cen") != std::string::npos || q.find("deneb") != std::string::npos ||
        q.find("arcturus") != std::string::npos || q.find("aldebaran") != std::string::npos ||
        q.find("canopus") != std::string::npos || q.find("spica") != std::string::npos ||
        q.find("antares") != std::string::npos || q.find("cygnus x") != std::string::npos ||
        q.find("star") != std::string::npos || q.rfind("hd ", 0) == 0 || q.rfind("hip ", 0) == 0 ||
        q.rfind("hr ", 0) == 0 || q.rfind("alf ", 0) == 0 || q.rfind("bet ", 0) == 0) {
        return true;
    }
    return false;
}

bool StellarCatalogProvider::searchObjects(const std::string& query, 
                                          std::vector<SearchResult>& outResults, 
                                          std::string& outError) {
    outResults.clear();
    if (query.empty()) return true;

    std::string cleanQ = query;
    std::transform(cleanQ.begin(), cleanQ.end(), cleanQ.begin(), ::tolower);
    if (cleanQ.rfind("star_", 0) == 0) cleanQ = cleanQ.substr(5);

    // 1. Check curated systems first
    for (const auto& kv : m_curatedSystems) {
        const auto& sys = kv.second;
        std::string sysLow = sys.systemName;
        std::transform(sysLow.begin(), sysLow.end(), sysLow.begin(), ::tolower);
        if (sysLow.find(cleanQ) != std::string::npos || cleanQ.find(sysLow) != std::string::npos ||
            (cleanQ == "sirius" && sys.systemName.find("Sirius") != std::string::npos) ||
            (cleanQ == "alpha centauri" && sys.systemName.find("Alpha Centauri") != std::string::npos)) {
            SearchResult res;
            res.sourceName = getProviderName();
            res.sourceId = "system_" + kv.first;
            res.name = sys.systemName;
            res.type = sys.type;
            res.details = sys.description;
            res.dataCompleteness = 1.0f;
            outResults.push_back(res);
        }
    }

    // 2. Check exact alias/catalog match first to rank exact matches at the very top
    std::string exactSlug;
    auto aliasIt = m_aliasToSlug.find(cleanQ);
    if (aliasIt != m_aliasToSlug.end()) {
        exactSlug = aliasIt->second;
        auto starIt = m_curatedStars.find(exactSlug);
        if (starIt != m_curatedStars.end()) {
            const auto& s = starIt->second;
            SearchResult res;
            res.sourceName = getProviderName();
            res.sourceId = "star_" + s.slug;
            res.name = s.name + " (" + s.mainAlias + ")";
            res.type = s.classification + " (" + s.spectralType + ")";
            res.aliases = s.crossIdents;
            res.dataCompleteness = 1.0f;

            char dBuf[128];
            snprintf(dBuf, sizeof(dBuf), "Mass: %.2f M☉ | Rad: %.2f R☉ | Teff: %.0f K | Lum: %.1f L☉ | Dist: %.1f ly", 
                     s.massMsun, s.radiusRsun, s.teffK, s.lumLsun, s.distanceLy);
            res.details = dBuf;
            outResults.push_back(res);
        }
    }

    // 3. Check curated stars for substring matches
    for (const auto& kv : m_curatedStars) {
        if (!exactSlug.empty() && kv.first == exactSlug) continue;
        const auto& s = kv.second;
        std::string nameLow = s.name;
        std::transform(nameLow.begin(), nameLow.end(), nameLow.begin(), ::tolower);
        std::string aliasLow = s.mainAlias;
        std::transform(aliasLow.begin(), aliasLow.end(), aliasLow.begin(), ::tolower);
        std::string crossLow = s.crossIdents;
        std::transform(crossLow.begin(), crossLow.end(), crossLow.begin(), ::tolower);

        if (nameLow.find(cleanQ) != std::string::npos || aliasLow.find(cleanQ) != std::string::npos ||
            crossLow.find(cleanQ) != std::string::npos || (cleanQ == "sirius" && s.name.find("Sirius") != std::string::npos)) {
            SearchResult res;
            res.sourceName = getProviderName();
            res.sourceId = "star_" + s.slug;
            res.name = s.name + " (" + s.mainAlias + ")";
            res.type = s.classification + " (" + s.spectralType + ")";
            res.aliases = s.crossIdents;
            res.dataCompleteness = 1.0f;

            char dBuf[128];
            snprintf(dBuf, sizeof(dBuf), "Mass: %.2f M☉ | Rad: %.2f R☉ | Teff: %.0f K | Lum: %.1f L☉ | Dist: %.1f ly", 
                     s.massMsun, s.radiusRsun, s.teffK, s.lumLsun, s.distanceLy);
            res.details = dBuf;
            outResults.push_back(res);
        }
    }

    if (!outResults.empty()) {
        return true;
    }

    // 3. Fallback: Query live CDS Sesame resolver for arbitrary catalog stars
    CelestialBodyRecord liveRec;
    bool liveOk = queryCDSSesame(query, liveRec, outResults, outError);
    return liveOk && !outResults.empty();
}

bool StellarCatalogProvider::queryCDSSesame(const std::string& query, 
                                           CelestialBodyRecord& outRecord, 
                                           std::vector<SearchResult>& outResults, 
                                           std::string& outError) {
    std::string cleanQuery = query;
    if (cleanQuery.rfind("star_", 0) == 0) cleanQuery = cleanQuery.substr(5);
    if (cleanQuery.rfind("system_", 0) == 0) cleanQuery = cleanQuery.substr(7);

    std::string url = getBaseUrl() + "?" + HttpClient::urlEncode(cleanQuery);
    auto resp = m_http.get(url, {}, 8);
    if (!resp.success || resp.body.empty()) {
        outError = "CDS Sesame resolution failed: " + resp.errorMessage;
        return false;
    }

    // Parse Sesame line output (%C.0, %S, %X, %J, %M.V, %I)
    std::stringstream ss(resp.body);
    std::string line;
    std::string otype, spType, mainName = cleanQuery;
    double plxMas = 0.0, vMag = 0.0;
    std::vector<std::string> identifiers;

    while (std::getline(ss, line)) {
        if (line.rfind("%C.0 ", 0) == 0) {
            otype = line.substr(5);
        } else if (line.rfind("%S ", 0) == 0) {
            std::stringstream sss(line.substr(3));
            sss >> spType;
        } else if (line.rfind("%X ", 0) == 0) {
            try { plxMas = std::stod(line.substr(3)); } catch (...) {}
        } else if (line.rfind("%M.V ", 0) == 0) {
            try { vMag = std::stod(line.substr(5)); } catch (...) {}
        } else if (line.rfind("%I.0 ", 0) == 0) {
            mainName = line.substr(5);
            if (mainName.rfind("* ", 0) == 0) mainName = mainName.substr(2);
        } else if (line.rfind("%I ", 0) == 0) {
            std::string id = line.substr(3);
            if (id.rfind("* ", 0) == 0) id = id.substr(2);
            identifiers.push_back(id);
        }
    }

    if (spType.empty() && plxMas <= 0.0 && otype.empty()) {
        outError = "No astronomical record resolved by CDS for: " + cleanQuery;
        return false;
    }

    // Determine Classification & Temperature
    double teffK = 5778.0;
    std::string autoClass = "Star (Main Sequence)";
    if (!spType.empty()) {
        char sClass = spType[0];
        if (sClass == 'O') teffK = 35000.0;
        else if (sClass == 'B') teffK = 18000.0;
        else if (sClass == 'A') teffK = 8800.0;
        else if (sClass == 'F') teffK = 6500.0;
        else if (sClass == 'G') teffK = 5600.0;
        else if (sClass == 'K') teffK = 4400.0;
        else if (sClass == 'M') teffK = 3200.0;
        else if (sClass == 'D') teffK = 20000.0;

        if (otype.find("WD") != std::string::npos || sClass == 'D') autoClass = "Star (White Dwarf)";
        else if (spType.find("Ia") != std::string::npos || spType.find("Ib") != std::string::npos || otype.find("s*r") != std::string::npos) autoClass = "Star (Supergiant)";
        else if (spType.find("III") != std::string::npos || spType.find("II") != std::string::npos) autoClass = "Star (Giant)";
        else if (sClass == 'M') autoClass = "Star (Red Dwarf)";
    }

    // Derive Physical Properties
    double distPc = (plxMas > 0.0) ? (1000.0 / plxMas) : 10.0;
    double distLy = distPc * 3.26156;
    double absMag = vMag - 5.0 * std::log10(std::max(1.0, distPc) / 10.0);
    double lumLsun = std::pow(10.0, -0.4 * (absMag - 4.74));
    if (lumLsun <= 0.0 || std::isnan(lumLsun)) lumLsun = 1.0;
    double rRsun = std::sqrt(lumLsun) * std::pow(5778.0 / teffK, 2.0);
    rRsun = std::clamp(rRsun, 0.005, 1500.0);
    double mMsun = std::pow(lumLsun, 0.28);
    mMsun = std::clamp(mMsun, 0.08, 150.0);

    // Build SearchResult
    SearchResult sr;
    sr.sourceName = getProviderName();
    sr.sourceId = "star_" + cleanQuery;
    sr.name = mainName + " (" + cleanQuery + ")";
    sr.type = autoClass + " (" + (spType.empty() ? "Stellar" : spType) + ")";
    char dBuf[128];
    snprintf(dBuf, sizeof(dBuf), "Mass: %.2f M☉ | Rad: %.2f R☉ | Teff: %.0f K | Lum: %.1f L☉ | Dist: %.1f ly", 
             mMsun, rRsun, teffK, lumLsun, distLy);
    sr.details = dBuf;
    sr.dataCompleteness = 0.90f;
    for (size_t i = 0; i < std::min((size_t)4, identifiers.size()); ++i) {
        if (!sr.aliases.empty()) sr.aliases += ", ";
        sr.aliases += identifiers[i];
    }
    outResults.push_back(sr);

    // Build outRecord
    outRecord.sourceName = getProviderName();
    outRecord.object.slug = cleanQuery;
    std::transform(outRecord.object.slug.begin(), outRecord.object.slug.end(), outRecord.object.slug.begin(), [](char c){
        return isalnum((unsigned char)c) ? (char)tolower(c) : '_';
    });
    outRecord.object.name = mainName;
    outRecord.object.type = (spType.empty() ? "Stellar Object" : (spType + " Star"));
    outRecord.object.classification = autoClass;
    outRecord.object.provenanceStatus = "Stellar Catalogue (CDS/SIMBAD Verified)";
    outRecord.object.category = mainName + " System";
    outRecord.object.color = VisualStateAdapter::temperatureToPlanckRGB(teffK);

    outRecord.physical.massKg = mMsun * UnitConverter::SOLAR_MASS_KG;
    outRecord.physical.radiusM = rRsun * UnitConverter::SOLAR_RADIUS_M;
    outRecord.physical.surfaceTempK = teffK;
    outRecord.physical.luminosityW = lumLsun * UnitConverter::SOLAR_LUMINOSITY_W;
    outRecord.physical.spectralType = spType;
    outRecord.physical.surfaceGravityMps2 = (UnitConverter::G_CONST * outRecord.physical.massKg.value()) / (outRecord.physical.radiusM.value() * outRecord.physical.radiusM.value());
    outRecord.physical.escapeVelocityMps = std::sqrt(2.0 * UnitConverter::G_CONST * outRecord.physical.massKg.value() / outRecord.physical.radiusM.value());
    outRecord.physical.sourceRecordId = mainName;
    outRecord.physical.atmosphereSummary = "Photosphere & Chromosphere (" + spType + ")";

    outRecord.stateVector.positionM = glm::dvec3(0.0);
    outRecord.stateVector.velocityMps = glm::dvec3(0.0);
    outRecord.stateVector.referenceFrame = "System-Barycentric";

    outRecord.composition = {
        { 0, 0, "Hydrogen (H)", 74.0f, {0.9f, 0.4f, 0.2f, 1.0f} },
        { 0, 0, "Helium (He)", 24.0f, {1.0f, 0.8f, 0.3f, 1.0f} },
        { 0, 0, "Heavier Metals", 2.0f, {0.7f, 0.7f, 0.7f, 1.0f} }
    };

    return true;
}

bool StellarCatalogProvider::fetchObjectData(const std::string& sourceIdOrName, 
                                            CelestialBodyRecord& outRecord, 
                                            std::string& outError) {
    std::string key = sourceIdOrName;
    if (key.rfind("star_", 0) == 0) key = key.substr(5);
    if (key.rfind("system_", 0) == 0) key = key.substr(7);
    std::string lowKey = key;
    std::transform(lowKey.begin(), lowKey.end(), lowKey.begin(), ::tolower);

    // Resolve alias if present
    if (m_aliasToSlug.find(lowKey) != m_aliasToSlug.end()) {
        lowKey = m_aliasToSlug[lowKey];
    }

    // Check Curated Stars
    auto it = m_curatedStars.find(lowKey);
    if (it != m_curatedStars.end()) {
        const auto& s = it->second;
        outRecord.sourceName = getProviderName();
        outRecord.object.slug = s.slug;
        outRecord.object.name = s.name;
        outRecord.object.type = s.spectralType + " Star";
        outRecord.object.classification = s.classification;
        outRecord.object.provenanceStatus = "Stellar Catalogue (Astrometric Baseline)";
        outRecord.object.category = s.systemName;
        outRecord.object.color = s.color;

        outRecord.physical.massKg = s.massMsun * UnitConverter::SOLAR_MASS_KG;
        outRecord.physical.radiusM = s.radiusRsun * UnitConverter::SOLAR_RADIUS_M;
        outRecord.physical.surfaceTempK = s.teffK;
        outRecord.physical.luminosityW = s.lumLsun * UnitConverter::SOLAR_LUMINOSITY_W;
        outRecord.physical.spectralType = s.spectralType;
        outRecord.physical.surfaceGravityMps2 = (UnitConverter::G_CONST * outRecord.physical.massKg.value()) / (outRecord.physical.radiusM.value() * outRecord.physical.radiusM.value());
        outRecord.physical.escapeVelocityMps = std::sqrt(2.0 * UnitConverter::G_CONST * outRecord.physical.massKg.value() / outRecord.physical.radiusM.value());
        outRecord.physical.meanDensityKgM3 = outRecord.physical.massKg.value() / ((4.0 / 3.0) * UnitConverter::PI * std::pow(outRecord.physical.radiusM.value(), 3.0));
        outRecord.physical.sourceRecordId = s.mainAlias;
        outRecord.physical.atmosphereSummary = "Photosphere & Chromosphere (" + s.spectralType + ")";

        // Barycentric State Vector
        outRecord.stateVector.epochJd = UnitConverter::J2000_JD;
        outRecord.stateVector.positionM = s.barycentricPosM;
        outRecord.stateVector.velocityMps = s.barycentricVelMps;
        outRecord.stateVector.referenceFrame = "System-Barycentric";

        outRecord.composition = {
            { 0, 0, "Hydrogen (H)", 74.0f, {0.9f, 0.4f, 0.2f, 1.0f} },
            { 0, 0, "Helium (He)", 24.0f, {1.0f, 0.8f, 0.3f, 1.0f} },
            { 0, 0, "Heavier Metals", 2.0f, {0.7f, 0.7f, 0.7f, 1.0f} }
        };

        return true;
    }

    // Check Curated Systems (fetch primary component)
    auto sysIt = m_curatedSystems.find(lowKey);
    if (sysIt != m_curatedSystems.end() && !sysIt->second.componentSlugs.empty()) {
        return fetchObjectData(sysIt->second.componentSlugs[0], outRecord, outError);
    }

    // Fallback: Query live CDS Sesame
    std::vector<SearchResult> unused;
    return queryCDSSesame(key, outRecord, unused, outError);
}

bool StellarCatalogProvider::fetchSystemComponents(const std::string& systemNameOrId,
                                                   std::vector<CelestialBodyRecord>& outComponents,
                                                   std::string& outError) {
    outComponents.clear();
    std::string key = systemNameOrId;
    if (key.rfind("system_", 0) == 0) key = key.substr(7);
    if (key.rfind("star_", 0) == 0) key = key.substr(5);
    std::string lowKey = key;
    std::transform(lowKey.begin(), lowKey.end(), lowKey.begin(), ::tolower);

    if (m_aliasToSlug.find(lowKey) != m_aliasToSlug.end()) {
        lowKey = m_aliasToSlug[lowKey];
    }

    // 1. Match in curated systems (exact or flexible)
    auto it = m_curatedSystems.find(lowKey);
    if (it == m_curatedSystems.end()) {
        if (lowKey.find("system") == std::string::npos) {
            it = m_curatedSystems.find(lowKey + "_system");
        }
    }
    if (it == m_curatedSystems.end()) {
        for (auto curIt = m_curatedSystems.begin(); curIt != m_curatedSystems.end(); ++curIt) {
            std::string sysLow = curIt->second.systemName;
            std::transform(sysLow.begin(), sysLow.end(), sysLow.begin(), ::tolower);
            if (sysLow.find(lowKey) != std::string::npos || lowKey.find(curIt->first) != std::string::npos || curIt->first.find(lowKey) != std::string::npos) {
                it = curIt;
                break;
            }
        }
    }
    if (it != m_curatedSystems.end()) {
        for (const auto& compSlug : it->second.componentSlugs) {
            CelestialBodyRecord compRec;
            std::string compErr;
            if (fetchObjectData(compSlug, compRec, compErr)) {
                outComponents.push_back(compRec);
            }
        }
        return !outComponents.empty();
    }

    // 2. Direct match as star belonging to a curated system
    auto starIt = m_curatedStars.find(lowKey);
    if (starIt != m_curatedStars.end()) {
        std::string sysName = starIt->second.systemName;
        std::string lowSys = sysName;
        std::transform(lowSys.begin(), lowSys.end(), lowSys.begin(), ::tolower);
        if (m_aliasToSlug.find(lowSys) != m_aliasToSlug.end()) {
            return fetchSystemComponents(m_aliasToSlug[lowSys], outComponents, outError);
        }
        // Single star system
        CelestialBodyRecord singleRec;
        if (fetchObjectData(lowKey, singleRec, outError)) {
            outComponents.push_back(singleRec);
            return true;
        }
    }

    // 3. Fallback: Live resolution of single star system
    CelestialBodyRecord rec;
    std::vector<SearchResult> unused;
    if (queryCDSSesame(key, rec, unused, outError)) {
        outComponents.push_back(rec);
        return true;
    }

    outError = "No stellar system found matching: " + systemNameOrId;
    return false;
}

bool StellarCatalogProvider::fetchEphemerisSeries(const std::string& sourceIdOrName, 
                                                 double startJd, 
                                                 double endJd, 
                                                 double stepDays, 
                                                 std::vector<EphemerisRecord>& outRecords, 
                                                 std::string& outError) {
    outRecords.clear();
    outError = "Stellar ephemerides are computed numerically using the AstroGenesis 1PN Einstein GR physics engine.";
    return false;
}

} // namespace AstroGenesis
