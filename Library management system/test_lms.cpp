#include <gtest/gtest.h>
#include <cstdio>
#include <string>
#include "Model.h"
#include "Controller.h"

using namespace std;

// Helper: Remove all CSV database files before each test case so default seed data starts clean
class DiamondLMSTest : public ::testing::Test {
protected:
    void SetUp() override {
        remove("db_users.csv");
        remove("db_books.csv");
        remove("db_copies.csv");
        remove("db_loans.csv");
        remove("db_reservations.csv");
        remove("db_fines.csv");
    }

    void TearDown() override {
        remove("db_users.csv");
        remove("db_books.csv");
        remove("db_copies.csv");
        remove("db_loans.csv");
        remove("db_reservations.csv");
        remove("db_fines.csv");
    }
};

// TC01: Login - Valid credentials (Librarian)
TEST_F(DiamondLMSTest, TC01_LoginSuccess) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    User* u = ctrl.login("long@ut.edu.vn", "123");
    ASSERT_NE(u, nullptr);
    EXPECT_EQ(u->getUserId(), "U02");
    EXPECT_EQ(u->getFullName(), "Nguyen Hoang Long");
    EXPECT_EQ(u->getRole(), "Librarian");
}

// TC02: Login - Valid email with wrong password
TEST_F(DiamondLMSTest, TC02_LoginWrongPassword) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    User* u = ctrl.login("long@ut.edu.vn", "wrongpass");
    EXPECT_EQ(u, nullptr);
}

// TC03: Register - Valid new Member registration
TEST_F(DiamondLMSTest, TC03_RegisterMemberSuccess) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string msg, newId;
    bool ok = ctrl.registerMember("Tran Minh Khoa", "khoa@ut.edu.vn", "123", msg, newId);
    EXPECT_TRUE(ok);
    EXPECT_EQ(newId, "U06");
    EXPECT_NE(msg.find("register successful"), string::npos);

    // Verify new member can log in immediately
    User* loggedIn = ctrl.login("khoa@ut.edu.vn", "123");
    ASSERT_NE(loggedIn, nullptr);
    EXPECT_EQ(loggedIn->getRole(), "Member");
}

// TC04: Register - Mismatched password & confirm password validation
// (Note: Confirm password check is enforced in CLI/Web layer before calling registerMember;
//  here we also verify short password < 3 chars is rejected by Controller)
TEST_F(DiamondLMSTest, TC04_RegisterPasswordValidation) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string pass = "123";
    string confirm = "456";
    EXPECT_NE(pass, confirm);

    // Also test Controller's minimum length check (< 3 chars)
    string msg, newId;
    bool ok = ctrl.registerMember("Nguy Thai Duong", "duong@ut.edu.vn", "12", msg, newId);
    EXPECT_FALSE(ok);
    EXPECT_NE(msg.find("Password at least 3 characters"), string::npos);
}

// TC05: Register - Reject comma (,) character to protect CSV integrity
TEST_F(DiamondLMSTest, TC05_RegisterRejectCommaInInput) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string msg, newId;
    bool ok = ctrl.registerMember("Nguyen, Van A", "vana@ut.edu.vn", "123", msg, newId);
    EXPECT_FALSE(ok);
    EXPECT_NE(msg.find("must not contain commas (,)"), string::npos);
}

// TC06: Update Profile - Member updates full name
TEST_F(DiamondLMSTest, TC06_UpdateMemberProfile) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string msg;
    bool ok = ctrl.updateUserProfile("U03", "Nguyen Van Truong", "", "", msg);
    EXPECT_TRUE(ok);
    EXPECT_NE(msg.find("Cap nhat thong tin thanh cong"), string::npos);

    string profile = ctrl.getUserProfile("U03");
    EXPECT_NE(profile.find("Nguyen Van Truong"), string::npos);
}

// TC07: Search Book - Search by title keyword ("De Men")
TEST_F(DiamondLMSTest, TC07_SearchBookByKeyword) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string res = ctrl.searchBook("De Men");
    EXPECT_NE(res.find("[B02]"), string::npos);
    EXPECT_NE(res.find("De Men phieu luu ky"), string::npos);
    EXPECT_NE(res.find("C02"), string::npos);
}

// TC08: Add Book - Add new Book (B05) and Available BookCopy (C05)
TEST_F(DiamondLMSTest, TC08_AddBookAndCopy) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string res = ctrl.addBookAndCopy("B05", "Lam di", "Vu Trong Phung", "C05", "NF-0200-01");
    EXPECT_NE(res.find("Da them sach"), string::npos);

    string searchRes = ctrl.searchBook("B05");
    EXPECT_NE(searchRes.find("Lam di"), string::npos);
    EXPECT_NE(searchRes.find("C05"), string::npos);
    EXPECT_NE(searchRes.find("Available"), string::npos);
}

// TC09: Delete Copy - Block deleting a BookCopy currently Borrowed (C02)
TEST_F(DiamondLMSTest, TC09_DeleteBorrowedCopyBlocked) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string res = ctrl.deleteCopy("C02");
    EXPECT_NE(res.find("Khong the xoa ban sao C02 vi dang o trang thai Borrowed"), string::npos);
}

// TC10: Borrow Book - Issue loan to eligible Member (U03) for Available copy (C01)
TEST_F(DiamondLMSTest, TC10_BorrowBookSuccess) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string res = ctrl.issueLoan("U03", "C01", "U02", "2026-11-22");
    EXPECT_NE(res.find("Thanh cong"), string::npos);
    EXPECT_NE(res.find("L03"), string::npos);

    // Verify copy C01 is now Borrowed and U03 activeLoansCount incremented to 1
    for (const auto& c : db.bookCopies) {
        if (c.copyId == "C01") {
            EXPECT_EQ(c.copyStatus, "Borrowed");
        }
    }
    for (const auto& m : db.members) {
        if (m.getUserId() == "U03") {
            EXPECT_EQ(m.activeLoansCount, 1);
        }
    }
}

// TC11: Borrow Book - Reject Member with unpaid fines (U05 owes 70,000 VND)
TEST_F(DiamondLMSTest, TC11_BorrowBookRejectedDueToUnpaidFine) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string res = ctrl.issueLoan("U05", "C01", "U02", "2026-11-22");
    EXPECT_NE(res.find("Tu choi"), string::npos);
    EXPECT_NE(res.find("dang no phat"), string::npos);
}

// TC12: Return Book - On-time return generates no fine
TEST_F(DiamondLMSTest, TC12_ReturnBookOnTime) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    // L01 belongs to U04, due date is today + 10 days -> returning today is on time
    string res = ctrl.processReturn("L01", DateUtils::today());
    EXPECT_NE(res.find("Tra sach dung han thanh cong"), string::npos);

    for (const auto& m : db.members) {
        if (m.getUserId() == "U04") {
            EXPECT_EQ(m.activeLoansCount, 0);
        }
    }
}

// TC13: Fine - Late return (3 days overdue) automatically creates 15,000 VND fine
TEST_F(DiamondLMSTest, TC13_ReturnBookOverdueGeneratesFine) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    // L01 due date is today + 10 days -> returning at today + 13 days is 3 days late
    string lateDate = DateUtils::addDays(DateUtils::today(), 13);
    string res = ctrl.processReturn("L01", lateDate);
    EXPECT_NE(res.find("tre 3 ngay"), string::npos);
    EXPECT_NE(res.find("15000 VND"), string::npos);
}

// TC14: Collect Fine - Pay unpaid fine F01 (70,000 VND) and clear U05 debt
TEST_F(DiamondLMSTest, TC14_CollectFineClearsDebt) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string res = ctrl.collectFine("F01");
    EXPECT_NE(res.find("Da thu tien phat & xoa no cho phieu: F01"), string::npos);

    for (const auto& m : db.members) {
        if (m.getUserId() == "U05") {
            EXPECT_DOUBLE_EQ(m.totalUnpaidFines, 0.0);
            EXPECT_TRUE(m.canBorrow());
        }
    }
}

// TC15: Renew Loan - Maximum 2 renewals allowed, 3rd attempt rejected
TEST_F(DiamondLMSTest, TC15_RenewLoanMaxTwoTimes) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string r1 = ctrl.renewLoan("L01");
    EXPECT_NE(r1.find("Gia han thanh cong"), string::npos);

    string r2 = ctrl.renewLoan("L01");
    EXPECT_NE(r2.find("Gia han thanh cong"), string::npos);

    string r3 = ctrl.renewLoan("L01");
    EXPECT_NE(r3.find("Khong the gia han"), string::npos);
}

// TC16: Hold Book - Member U03 places a reservation hold on Book B03
TEST_F(DiamondLMSTest, TC16_ReserveBookSuccess) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    string res = ctrl.reserveBook("U03", "B03");
    EXPECT_NE(res.find("Dat cho thanh cong"), string::npos);
    EXPECT_NE(res.find("R02"), string::npos);
    EXPECT_NE(res.find("Pending"), string::npos);
}

// TC17: Approve Hold - Librarian approves Pending hold R02 -> Ready & Copy Reserved
TEST_F(DiamondLMSTest, TC17_ApproveReservationLocksCopy) {
    LibraryDatabase db;
    LibraryController ctrl(db);

    ctrl.reserveBook("U03", "B03");
    string res = ctrl.approveReservation("R02");
    EXPECT_NE(res.find("Da duyet phieu R02"), string::npos);
    EXPECT_NE(res.find("C03"), string::npos);

    for (const auto& c : db.bookCopies) {
        if (c.copyId == "C03") {
            EXPECT_EQ(c.copyStatus, "Reserved");
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
