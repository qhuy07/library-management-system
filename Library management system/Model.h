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
class User {
protected:
    string userId;
    string fullName;
    string email;
    string passwordHash;
    string status;
    string role;

public:
    User(string id = "", string name = "", string mail = "", string pwd = "", string st = "Active", string r = "User")
        : userId(id), fullName(name), email(mail), passwordHash(pwd), status(st), role(r) {}

    virtual ~User() {}

    bool login(const string& inputEmail, const string& pwd) {
        return (email == inputEmail && passwordHash == pwd && status == "Active");
    }
    void logout() {}
    void updateProfile(const string& newName) { fullName = newName; }
    void updateEmail(const string& newEmail) { email = newEmail; }
    void updatePassword(const string& newPwd) { passwordHash = newPwd; }

    string getUserId() const { return userId; }
    string getFullName() const { return fullName; }
    string getEmail() const { return email; }
    string getPasswordHash() const { return passwordHash; }
    string getStatus() const { return status; }
    string getRole() const { return role; }
};

class Member : public User {
public:
    string membershipCardNo;
    int maxQuota;
    int activeLoansCount;
    double totalUnpaidFines;

    Member(string id = "", string name = "", string mail = "", string pwd = "",
           string cardNo = "", int quota = 3, int active = 0, double fines = 0.0)
        : User(id, name, mail, pwd, "Active", "Member"),
          membershipCardNo(cardNo), maxQuota(quota), activeLoansCount(active), totalUnpaidFines(fines) {}

    bool canBorrow() const {
        return (status == "Active" && activeLoansCount < maxQuota && totalUnpaidFines <= 0.0);
    }
};

class Librarian : public User {
public:
    string staffCode;
    string deskShift;

    Librarian(string id = "", string name = "", string mail = "", string pwd = "",
              string code = "", string shift = "Morning")
        : User(id, name, mail, pwd, "Active", "Librarian"),
          staffCode(code), deskShift(shift) {}
};

class Administrator : public User {
public:
    int adminLevel;
    string lastAuditDate;

    Administrator(string id = "", string name = "", string mail = "", string pwd = "",
                  int level = 1, string auditDate = DateUtils::today())
        : User(id, name, mail, pwd, "Active", "Administrator"),
          adminLevel(level), lastAuditDate(auditDate) {}
};


class Category {
public:
    string categoryId;
    string categoryName;
    string shelfZone;

    void updateShelfZone(const string& newZone) { shelfZone = newZone; }
};

class Book {
public:
    string bookId;
    string isbn;
    string title;
    string author;
    string publisher;
    int pubYear;
    string categoryId;

    void updateMetadata(const string& t, const string& a, const string& p) {
        title = t; author = a; publisher = p;
    }
};

class BookCopy {
public:
    string copyId;
    string barcode;
    string rackLocation;
    string copyStatus;
    string bookId;

    void markBorrowed() { copyStatus = "Borrowed"; }
    void markReturned() { copyStatus = "Available"; }
    void markReserved() { copyStatus = "Reserved"; }
    bool isAvailable() const { return copyStatus == "Available"; }
    bool isReserved()  const { return copyStatus == "Reserved"; }
};

class Loan {
public:
    string loanId;
    string memberId;
    string librarianId;
    string copyId;
    string issueDate;   // YYYY-MM-DD
    string dueDate;     // YYYY-MM-DD
    string returnDate;  // YYYY-MM-DD hoac "NONE"
    int renewalCount;
    string status;

    bool isOverdue(const string& currentDate = DateUtils::today()) const {
        return (status != "Returned" && DateUtils::diffDays(dueDate, currentDate) > 0);
    }
    int calculateOverdueDays(const string& checkDate) const {
        int diff = DateUtils::diffDays(dueDate, checkDate);
        return (diff > 0) ? diff : 0;
    }
    bool renewLoan(int extraDays) {
        if (renewalCount < 2 && status == "Active") {
            dueDate = DateUtils::addDays(dueDate, extraDays);
            renewalCount++;
            return true;
        }
        return false;
    }
};
class Reservation {
public:
    string reservationId;
    string requestDate;
    string expiryDate;
    string status;          // Pending | Ready | Fulfilled | Cancelled | Expired
    string memberId;
    string bookId;
    string copyId = "NONE"; // Ma ban sao dang duoc giu (neu status == Ready/Fulfilled)

    void markReady(const string& heldCopyId, const string& newExpiry) {
        copyId = heldCopyId;
        expiryDate = newExpiry;
        status = "Ready";
    }
    void cancelHold()  { status = "Cancelled"; }
    void fulfillHold() { status = "Fulfilled"; }
    void markExpired() { status = "Expired"; }
};
class Fine {
public:
    string fineId;
    int daysOverdue;
    double amount;
    string paidStatus;
    string paymentDate;
    string loanId;

    void computeFee(double rate) { amount = daysOverdue * rate; }
    void recordPayment(const string& dateStr) {
        paidStatus = "Paid";
        paymentDate = dateStr;
    }
};

class LibraryDatabase {
public:
    vector<Member> members;
    vector<Librarian> librarians;
    vector<Administrator> admins;
    vector<Category> categories;
    vector<Book> books;
    vector<BookCopy> bookCopies;
    vector<Loan> loans;
    vector<Reservation> reservations;
    vector<Fine> fines;

    int defaultLoanDays = 14;
    double fineRatePerDay = 5000.0;

    LibraryDatabase() {
        loadAll();
    }

    void seedDefaultData() {
        string todayStr = DateUtils::today();

        admins = {
            Administrator("U01", "Trinh Quang Huy", "admin@ut.edu.vn", "123", 1, todayStr)
        };
        librarians = {
            Librarian("U02", "Nguyen Hoang Long", "long@ut.edu.vn", "123", "STF-01", "Morning")
        };
        members = {
            Member("U03", "Nguyen Van Truong", "truong@ut.edu.vn", "123", "CARD-101", 3, 0, 0.0),
            Member("U04", "Nguy Thai Duong", "duong@ut.edu.vn", "123", "CARD-102", 3, 1, 0.0),
            Member("U05", "Van Cong Toan", "toan@ut.edu.vn", "123", "CARD-103", 3, 1, 70000.0)
        };
        categories = {
            {"CAT01", "Van hoc Viet Nam", "Zone A"},
            {"CAT02", "Cong nghe Phan mem", "Zone B"}
        };
        books = {
            {"B01", "978-604-1", "Vo nhat", "Kim Lan", "NXB Kim Dong", 2020, "CAT01"},
            {"B02", "978-604-2", "De Men phieu luu ky", "To Hoai", "NXB Kim Dong", 2019, "CAT01"},
            {"B03", "978-604-3", "Mat biec", "Nguyen Nhat Anh", "NXB Tre", 2021, "CAT01"},
            {"B04", "978-604-4", "So do", "Vu Trong Phung", "NXB Van Hoc", 2018, "CAT01"}
        };
        bookCopies = {
            {"C01", "NF-0121-03", "A1-01", "Available", "B01"},
            {"C02", "NF-0042-02", "A1-02", "Borrowed",  "B02"},
            {"C03", "NF-0075-01", "A1-03", "Available", "B03"},
            {"C04", "NF-0088-01", "A1-04", "Borrowed",  "B04"}
        };
        loans = {
            {"L01", "U04", "U02", "C02", DateUtils::addDays(todayStr, -4),  DateUtils::addDays(todayStr, 10),  "NONE", 0, "Active"},
            {"L02", "U05", "U02", "C04", DateUtils::addDays(todayStr, -28), DateUtils::addDays(todayStr, -14), "NONE", 0, "Overdue"}
        };
        reservations = {
            {"R01", DateUtils::addDays(todayStr, -1), DateUtils::addDays(todayStr, 2), "Pending", "U03", "B02", "NONE"}
        };
        fines = {
            {"F01", 14, 70000.0, "Unpaid", "NONE", "L02"}
        };
        saveAll();
    }

    void loadAll() {
        ifstream fCheck("db_users.csv");
        if (!fCheck.is_open()) {
            seedDefaultData();
            return;
        }
        fCheck.close();

        members.clear(); librarians.clear(); admins.clear();
        ifstream fu("db_users.csv");
        string line;
        while (getline(fu, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string role, id, name, mail, pwd, extra1, extra2, extra3;
            getline(ss, role, ','); getline(ss, id, ','); getline(ss, name, ',');
            getline(ss, mail, ','); getline(ss, pwd, ',');
            getline(ss, extra1, ','); getline(ss, extra2, ','); getline(ss, extra3, ',');
            if (role == "Member")
                members.push_back(Member(id, name, mail, pwd, extra1, 3, stoi(extra2), stod(extra3)));
            else if (role == "Librarian")
                librarians.push_back(Librarian(id, name, mail, pwd, extra1, extra2));
            else if (role == "Administrator")
                admins.push_back(Administrator(id, name, mail, pwd, stoi(extra1), extra2));
        }
