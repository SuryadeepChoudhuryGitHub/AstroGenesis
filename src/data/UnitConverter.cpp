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

} // namespace AstroGenesis
