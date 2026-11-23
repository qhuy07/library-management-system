#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <cmath>

using namespace std;

//TIEN ICH NGAY THANG THUC TE
namespace DateUtils {
   inline tm getLocalTm(time_t t) {
        tm localTm = {};
#if defined(_MSC_VER)
        localtime_s(&localTm, &t);
#elif defined(_WIN32)
        tm* p = localtime(&t);
        if (p) localTm = *p;
#else
        localtime_r(&t, &localTm);
#endif
        return localTm;
    }

    //Lay ngay hien tai dang YYYY-MM-DD
    inline string today() {
        time_t now = time(nullptr);
        tm lt = getLocalTm(now);
        ostringstream oss;
        oss << put_time(&lt, "%Y-%m-%d");
        return oss.str();
    }

    //Lay ngay gio hien tai dang YYYY-MM-DD HH:MM:SS
    inline string nowDateTime() {
        time_t now = time(nullptr);
        tm lt = getLocalTm(now);
        ostringstream oss;
        oss << put_time(&lt, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    inline bool parseDate(const string& dateStr, tm& outTm) {
        if (dateStr.size() != 10 || dateStr[4] != '-' || dateStr[7] != '-') return false;
        int y = 0, m = 0, d = 0;
        try {
            y = stoi(dateStr.substr(0, 4));
            m = stoi(dateStr.substr(5, 2));
            d = stoi(dateStr.substr(8, 2));
        } catch (...) {
            return false;
        }
        if (m < 1 || m > 12 || d < 1 || d > 31) return false;
        outTm = {};
        outTm.tm_year = y - 1900;
        outTm.tm_mon  = m - 1;
        outTm.tm_mday = d;
        outTm.tm_hour = 12; //Tranh loi lech gio DST
        outTm.tm_isdst = -1;
        return true;
    }

    inline bool isValidDate(const string& dateStr) {
        tm t = {};
        return parseDate(dateStr, t);
    }

    //Cong/tru so ngay vao chuoi YYYY-MM-DD
    inline string addDays(const string& dateStr, int days) {
        tm t = {};
        if (!parseDate(dateStr, t)) return today();
        t.tm_mday += days;
        time_t tt = mktime(&t);
        tm res = getLocalTm(tt);
        ostringstream oss;
        oss << put_time(&res, "%Y-%m-%d");
        return oss.str();
    }

    //Tinh so ngay giua 2 moc (endDate - startDate)
    inline int diffDays(const string& startDate, const string& endDate) {
        tm t1 = {}, t2 = {};
        if (!parseDate(startDate, t1) || !parseDate(endDate, t2)) return 0;
        time_t time1 = mktime(&t1);
        time_t time2 = mktime(&t2);
        double seconds = difftime(time2, time1);
        return static_cast<int>(round(seconds / 86400.0));
    }
}
