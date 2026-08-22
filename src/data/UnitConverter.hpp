#pragma once

#include <string>
#include <cmath>

namespace AstroGenesis {

class UnitConverter {
public:
    // Fundamental Astronomical & Physical Constants
    static constexpr double AU_TO_METERS       = 149597870700.0;    // 1 AU in meters (IAU 2012)
    static constexpr double KM_TO_METERS       = 1000.0;
    static constexpr double METERS_TO_KM       = 1.0 / 1000.0;
    static constexpr double METERS_TO_AU       = 1.0 / AU_TO_METERS;
    static constexpr double AU_TO_KM           = 149597870.7;
    static constexpr double KM_TO_AU           = 1.0 / AU_TO_KM;
    static constexpr double LY_TO_METERS       = 9.4607304725808e15;
    static constexpr double PARSEC_TO_METERS   = 3.08567758149137e16;
    static constexpr double PARSEC_TO_AU       = 206264.806247096;

    // Mass Constants (kg)
    static constexpr double EARTH_MASS_KG      = 5.97219e24;
    static constexpr double JUPITER_MASS_KG    = 1.89813e27;
    static constexpr double SOLAR_MASS_KG      = 1.98847e30;
    static constexpr double LUNAR_MASS_KG      = 7.342e22;

    // Radius Constants (meters)
    static constexpr double EARTH_RADIUS_M     = 6371000.0;
    static constexpr double JUPITER_RADIUS_M   = 69911000.0;
    static constexpr double SOLAR_RADIUS_M     = 696340000.0;
    static constexpr double LUNAR_RADIUS_M     = 1737400.0;

    // Time Constants
    static constexpr double SEC_PER_DAY        = 86400.0;
    static constexpr double DAYS_PER_JULIAN_YR = 365.25;
    static constexpr double J2000_JD           = 2451545.0; // 2000-01-01 12:00:00 TT

    // Speed of Light & Gravity
    static constexpr double G_CONST            = 6.67430e-11; // m^3 kg^-1 s^-2
    static constexpr double SPEED_OF_LIGHT     = 299792458.0; // m/s
    static constexpr double SOLAR_LUMINOSITY_W = 3.828e26;    // Watts

    // Math Constants
    static constexpr double PI                 = 3.14159265358979323846;
    static constexpr double DEG_TO_RAD         = PI / 180.0;
    static constexpr double RAD_TO_DEG         = 180.0 / PI;
    static constexpr double ARCSEC_TO_RAD      = (PI / 180.0) / 3600.0;
    static constexpr double RAD_TO_ARCSEC      = (180.0 / PI) * 3600.0;

    // Conversions
    static double auToMeters(double au) { return au * AU_TO_METERS; }
    static double metersToAu(double meters) { return meters * METERS_TO_AU; }
    static double kmToMeters(double km) { return km * KM_TO_METERS; }
    static double metersToKm(double meters) { return meters * METERS_TO_KM; }

    static double kmPerSecToMetersPerSec(double kmPerSec) { return kmPerSec * 1000.0; }
    static double auPerDayToMetersPerSec(double auPerDay) { return (auPerDay * AU_TO_METERS) / SEC_PER_DAY; }
    static double metersPerSecToKmPerSec(double mPerSec) { return mPerSec / 1000.0; }

    static double earthMassToKg(double mEarth) { return mEarth * EARTH_MASS_KG; }
    static double jupiterMassToKg(double mJup) { return mJup * JUPITER_MASS_KG; }
    static double solarMassToKg(double mSun) { return mSun * SOLAR_MASS_KG; }
    static double kgToEarthMass(double kg) { return kg / EARTH_MASS_KG; }
    static double kgToJupiterMass(double kg) { return kg / JUPITER_MASS_KG; }
    static double kgToSolarMass(double kg) { return kg / SOLAR_MASS_KG; }

    static double earthRadiusToMeters(double rEarth) { return rEarth * EARTH_RADIUS_M; }
    static double jupiterRadiusToMeters(double rJup) { return rJup * JUPITER_RADIUS_M; }
    static double solarRadiusToMeters(double rSun) { return rSun * SOLAR_RADIUS_M; }
    static double metersToEarthRadius(double meters) { return meters / EARTH_RADIUS_M; }

    static double degToRad(double deg) { return deg * DEG_TO_RAD; }
    static double radToDeg(double rad) { return rad * RAD_TO_DEG; }

    // Julian Date <-> Calendar UTC String
    static double calendarToJulianDate(int year, int month, int day, int hour = 0, int minute = 0, double second = 0.0);
    static std::string julianDateToUtcString(double jd);
    static double iso8601ToJulianDate(const std::string& isoStr);

    // High-Precision Keplerian Elements to 3D Cartesian State Vector (in AstroGenesis X-Z ecliptic plane, Y-up)
    static void keplerianToCartesian(double aMeters,
                                    double eccentricity,
                                    double inclinationDeg,
                                    double longAscNodeDeg,
                                    double argPeriapsisDeg,
                                    double meanAnomalyDeg,
                                    double centralMassKg,
                                    double bodyMassKg,
                                    double& outPosX, double& outPosY, double& outPosZ,
                                    double& outVelX, double& outVelY, double& outVelZ);

    // Formatted presentation strings
    static std::string formatDistance(double meters);
    static std::string formatMass(double kg);
    static std::string formatVelocity(double metersPerSec);
    static std::string formatPeriod(double seconds);
    static std::string formatAngleDeg(double degrees);
    static std::string formatPressure(double pascals);
    static std::string formatDensity(double kgM3);
};

} // namespace AstroGenesis
