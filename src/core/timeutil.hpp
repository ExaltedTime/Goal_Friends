// Calendar helpers on plain integers so tests never depend on the system clock.
// A "day" is the number of days since 1970-01-01 in local time (0 = Thursday).
#pragma once
#include <cstdint>

namespace tu {

struct Time {
    int day = 0;     // days since 1970-01-01 (local)
    int minute = 0;  // minute of day, 0..1439
    long long abs() const { return static_cast<long long>(day) * 1440 + minute; }
};

inline Time fromAbs(long long a) {
    long long d = a / 1440, m = a % 1440;
    if (m < 0) { m += 1440; --d; }
    return Time{static_cast<int>(d), static_cast<int>(m)};
}

// Howard Hinnant's civil date algorithms.
inline int daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

struct Civil { int y, m, d; };
inline Civil civilFromDays(int z) {
    z += 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int y = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned d = doy - (153 * mp + 2) / 5 + 1;
    const unsigned m = mp < 10 ? mp + 3 : mp - 9;
    return Civil{y + (m <= 2), static_cast<int>(m), static_cast<int>(d)};
}

inline int weekday(int day) { return ((day + 4) % 7 + 7) % 7; }  // 0 = Sunday

inline bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
inline int daysInMonth(int y, int m) {
    static const int dm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : dm[m - 1];
}

}  // namespace tu
