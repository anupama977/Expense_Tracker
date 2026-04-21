#include "httplib.h"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// ── OOP: Abstraction ──────────────────────────────────────────────────────────
// `Expense` defines the common interface for every expense type.
class Expense {
private:
    // OOP: Encapsulation
    double amount;
    string description;

public:
    Expense(double amt, const string& desc)
        : amount(amt < 0 ? 0 : amt), description(desc) {}

    virtual ~Expense() {}

    double getAmount()      const { return amount; }
    string getDescription() const { return description; }

    // OOP: Polymorphism – each derived class implements these differently.
    virtual string getCategory() const = 0;
    virtual string getKey()      const = 0;
};

// ── OOP: Inheritance ──────────────────────────────────────────────────────────
class FoodExpense : public Expense {
public:
    FoodExpense(double amt, const string& desc) : Expense(amt, desc) {}
    string getCategory() const override { return "Food"; }
    string getKey()      const override { return "food"; }
};

class TravelExpense : public Expense {
public:
    TravelExpense(double amt, const string& desc) : Expense(amt, desc) {}
    string getCategory() const override { return "Travel"; }
    string getKey()      const override { return "travel"; }
};

class EducationExpense : public Expense {
public:
    EducationExpense(double amt, const string& desc) : Expense(amt, desc) {}
    string getCategory() const override { return "Education"; }
    string getKey()      const override { return "education"; }
};

class ShoppingExpense : public Expense {
public:
    ShoppingExpense(double amt, const string& desc) : Expense(amt, desc) {}
    string getCategory() const override { return "Shopping"; }
    string getKey()      const override { return "shopping"; }
};

// Factory – returns heap-allocated object or nullptr for unknown key
Expense* createExpense(const string& key, double amount, const string& desc) {
    if (key == "food")      return new FoodExpense(amount, desc);
    if (key == "travel")    return new TravelExpense(amount, desc);
    if (key == "education") return new EducationExpense(amount, desc);
    if (key == "shopping")  return new ShoppingExpense(amount, desc);
    return nullptr;
}

// ── ExpenseTracker ────────────────────────────────────────────────────────────
class ExpenseTracker {
public:
    // OOP: Association – the tracker owns its Expense objects
    vector<Expense*> expenses;

    ~ExpenseTracker() { clear(); }

    // FIX: copy constructor and assignment operator so the tracker can be
    //      stored in std::map without double-deleting on move/copy.
    //      We use a simple deep-copy strategy.
    ExpenseTracker() {}

    ExpenseTracker(const ExpenseTracker& other) {
        for (size_t i = 0; i < other.expenses.size(); ++i) {
            Expense* e = other.expenses[i];
            expenses.push_back(createExpense(e->getKey(), e->getAmount(), e->getDescription()));
        }
    }

    ExpenseTracker& operator=(const ExpenseTracker& other) {
        if (this != &other) {
            clear();
            for (size_t i = 0; i < other.expenses.size(); ++i) {
                Expense* e = other.expenses[i];
                expenses.push_back(createExpense(e->getKey(), e->getAmount(), e->getDescription()));
            }
        }
        return *this;
    }

    void clear() {
        for (size_t i = 0; i < expenses.size(); ++i) {
            delete expenses[i];
        }
        expenses.clear();
    }

    void addExpense(Expense* expense) {
        if (expense) {
            expenses.push_back(expense);
        }
    }

    void removeExpense(int index) {
        if (index >= 0 && index < static_cast<int>(expenses.size())) {
            delete expenses[index];
            expenses.erase(expenses.begin() + index);
        }
    }

    int count() const {
        return static_cast<int>(expenses.size());
    }

    double total() const {
        double sum = 0;
        for (size_t i = 0; i < expenses.size(); ++i) {
            sum += expenses[i]->getAmount();
        }
        return sum;
    }

    double totalByKey(const string& key) const {
        double sum = 0;
        for (size_t i = 0; i < expenses.size(); ++i) {
            if (expenses[i]->getKey() == key) {
                sum += expenses[i]->getAmount();
            }
        }
        return sum;
    }
};

// ── OOP: Composition ──────────────────────────────────────────────────────────
// Each StudentSession owns its own ExpenseTracker.
struct StudentSession {
    string        studentName;
    ExpenseTracker tracker;
};

// ── Globals ───────────────────────────────────────────────────────────────────
map<string, StudentSession> sessions;
int sessionCounter = 1;

// ── Utility helpers ───────────────────────────────────────────────────────────
string readFile(const string& path) {
    ifstream file(path.c_str(), ios::in | ios::binary);
    if (!file) return "";
    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string urlDecode(const string& str) {
    string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            string hex = str.substr(i + 1, 2);
            result += static_cast<char>(strtol(hex.c_str(), nullptr, 16));
            i += 2;
        } else {
            result += str[i];
        }
    }
    return result;
}

string getFormValue(const string& body, const string& key) {
    string search = key + "=";
    size_t pos = body.find(search);
    if (pos == string::npos) return "";
    pos += search.size();
    size_t end = body.find('&', pos);
    string raw = (end == string::npos) ? body.substr(pos) : body.substr(pos, end - pos);
    return urlDecode(raw);
}

string escapeHtml(const string& text) {
    string out;
    for (size_t i = 0; i < text.size(); ++i) {
        switch (text[i]) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += text[i];  break;
        }
    }
    return out;
}

string formatAmount(double amount) {
    ostringstream out;
    out << fixed << setprecision(2) << amount;
    return out.str();
}

// FIX: replaceAll variant so every occurrence of a token is replaced,
//      not just the first one (original code only replaced once).
string replaceToken(string html, const string& token, const string& value) {
    size_t pos = 0;
    while ((pos = html.find(token, pos)) != string::npos) {
        html.replace(pos, token.size(), value);
        pos += value.size();
    }
    return html;
}

string getCookieValue(const httplib::Request& req, const string& key) {
    string cookieHeader = req.get_header_value("Cookie");
    if (cookieHeader.empty()) return "";
    string search = key + "=";
    size_t start = cookieHeader.find(search);
    if (start == string::npos) return "";
    start += search.size();
    size_t end = cookieHeader.find(';', start);
    return cookieHeader.substr(start, end == string::npos ? string::npos : end - start);
}

StudentSession* getSession(const httplib::Request& req) {
    string sessionId = getCookieValue(req, "session_id");
    if (sessionId.empty()) return nullptr;
    map<string, StudentSession>::iterator it = sessions.find(sessionId);
    if (it == sessions.end()) return nullptr;
    return &(it->second);
}

string createSessionId() {
    ostringstream out;
    out << "student_" << time(nullptr) << "_" << sessionCounter++;
    return out.str();
}

string buildMessage(const string& message, const string& cssClass) {
    if (message.empty()) return "";
    return "<p class='" + cssClass + "'>" + escapeHtml(message) + "</p>";
}

// ── HTML section builders ─────────────────────────────────────────────────────
string buildSummarySection(const ExpenseTracker& tracker) {
    if (tracker.count() == 0) return "";
    string html;
    html += "<section class='card'>";
    html += "<h2>Summary</h2>";
    html += "<div class='summary-grid'>";
    html += "<div class='summary-box'><span>Total</span><strong>Rs. "     + formatAmount(tracker.total())                   + "</strong></div>";
    html += "<div class='summary-box'><span>Food</span><strong>Rs. "      + formatAmount(tracker.totalByKey("food"))        + "</strong></div>";
    html += "<div class='summary-box'><span>Travel</span><strong>Rs. "    + formatAmount(tracker.totalByKey("travel"))      + "</strong></div>";
    html += "<div class='summary-box'><span>Education</span><strong>Rs. " + formatAmount(tracker.totalByKey("education"))   + "</strong></div>";
    html += "<div class='summary-box'><span>Shopping</span><strong>Rs. "  + formatAmount(tracker.totalByKey("shopping"))    + "</strong></div>";
    html += "</div></section>";
    return html;
}

string buildExpenseListSection(const ExpenseTracker& tracker) {
    string html = "<div class='history-list'>";
    if (tracker.count() == 0) {
        html += "<p class='empty-message'>No expenses added yet.</p>";
    } else {
        html += "<div class='expense-list'>";
        for (size_t i = 0; i < tracker.expenses.size(); ++i) {
            Expense* e = tracker.expenses[i];
            html += "<div class='expense-item'>";
            html += "<div>";
            html += "<p class='expense-category'>"    + escapeHtml(e->getCategory())   + "</p>";
            html += "<p class='expense-description'>" + escapeHtml(e->getDescription())+ "</p>";
            html += "</div>";
            html += "<div class='expense-actions'>";
            html += "<span class='expense-amount'>Rs. " + formatAmount(e->getAmount()) + "</span>";
            html += "<form method='POST' action='/delete'>";
            html += "<input type='hidden' name='index' value='" + to_string(i) + "'>";
            html += "<button type='submit' class='delete-button'>Delete</button>";
            html += "</form></div></div>";
        }
        html += "</div>";
    }
    html += "</div>";
    return html;
}

string buildBillSection(const StudentSession& session) {
    string html;
    html += "<div class='bill-summary'>";
    html += "<p class='bill-name'>Student: <strong>" + escapeHtml(session.studentName) + "</strong></p>";

    if (session.tracker.count() == 0) {
        html += "<p class='empty-message'>No expenses available for the bill.</p>";
    } else {
        html += "<table class='bill-table'>";
        html += "<thead><tr><th>No.</th><th>Category</th><th>Description</th><th>Amount</th></tr></thead>";
        html += "<tbody>";
        for (size_t i = 0; i < session.tracker.expenses.size(); ++i) {
            Expense* e = session.tracker.expenses[i];
            html += "<tr>";
            html += "<td>" + to_string(i + 1)                          + "</td>";
            html += "<td>" + escapeHtml(e->getCategory())               + "</td>";
            html += "<td>" + escapeHtml(e->getDescription())            + "</td>";
            html += "<td>Rs. " + formatAmount(e->getAmount())           + "</td>";
            html += "</tr>";
        }
        html += "</tbody></table>";
        html += "<div class='bill-total'>Grand Total: <strong>Rs. " + formatAmount(session.tracker.total()) + "</strong></div>";
    }

    html += "</div>";
    return html;
}

// ── Page builders ─────────────────────────────────────────────────────────────
string buildLoginPage(const string& messageHtml) {
    string html = readFile("login.html");
    if (html.empty()) return "<h1>login.html not found.</h1>";
    html = replaceToken(html, "{{TITLE}}",   "Student Login");
    html = replaceToken(html, "{{MESSAGE}}", messageHtml);
    return html;
}

string buildHomePage(const StudentSession& session, const string& messageHtml) {
    string html = readFile("index.html");
    if (html.empty()) return "<h1>index.html not found.</h1>";

    string welcome = "<div class='top-bar'>"
        "<p>Welcome, <strong>" + escapeHtml(session.studentName) + "</strong></p>"
        "<form method='POST' action='/logout'>"
        "<button type='submit' class='small-button'>Log Out</button></form></div>";

    html = replaceToken(html, "{{TITLE}}",        "Student Dashboard");
    html = replaceToken(html, "{{WELCOME}}",      welcome);
    html = replaceToken(html, "{{MESSAGE}}",      messageHtml);
    html = replaceToken(html, "{{STUDENT_NAME}}", escapeHtml(session.studentName));
    return html;
}

string buildExpensePage(const StudentSession& session, const string& messageHtml) {
    string html = readFile("expense.html");
    if (html.empty()) return "<h1>expense.html not found.</h1>";

    string welcome = "<div class='top-bar'>"
        "<p>Welcome, <strong>" + escapeHtml(session.studentName) + "</strong></p>"
        "<form method='POST' action='/logout'>"
        "<button type='submit' class='small-button'>Log Out</button></form></div>";

    html = replaceToken(html, "{{TITLE}}",   "Add Expense");
    html = replaceToken(html, "{{WELCOME}}", welcome);
    html = replaceToken(html, "{{MESSAGE}}", messageHtml);
    html = replaceToken(html, "{{SUMMARY}}", buildSummarySection(session.tracker));
    return html;
}

string buildHistoryPage(const StudentSession& session, const string& messageHtml) {
    string html = readFile("history.html");
    if (html.empty()) return "<h1>history.html not found.</h1>";

    string welcome = "<div class='top-bar'>"
        "<p>History for <strong>" + escapeHtml(session.studentName) + "</strong></p>"
        "<form method='POST' action='/logout'>"
        "<button type='submit' class='small-button'>Log Out</button></form></div>";

    html = replaceToken(html, "{{TITLE}}",        "Expense History");
    html = replaceToken(html, "{{WELCOME}}",      welcome);
    html = replaceToken(html, "{{MESSAGE}}",      messageHtml);
    html = replaceToken(html, "{{EXPENSE_LIST}}", buildExpenseListSection(session.tracker));
    return html;
}

string buildBillPage(const StudentSession& session, const string& messageHtml) {
    string html = readFile("bill.html");
    if (html.empty()) return "<h1>bill.html not found.</h1>";

    string welcome = "<div class='top-bar'>"
        "<p>Bill for <strong>" + escapeHtml(session.studentName) + "</strong></p>"
        "<form method='POST' action='/logout'>"
        "<button type='submit' class='small-button'>Log Out</button></form></div>";

    html = replaceToken(html, "{{TITLE}}",   "Total Bill");
    html = replaceToken(html, "{{WELCOME}}", welcome);
    html = replaceToken(html, "{{MESSAGE}}", messageHtml);
    html = replaceToken(html, "{{BILL}}",    buildBillSection(session));
    return html;
}

// ── Redirect helper ───────────────────────────────────────────────────────────
void redirectToLogin(httplib::Response& res) {
    res.set_redirect("/login");
}

// ── main ──────────────────────────────────────────────────────────────────────
int main() {
    httplib::Server server;

    // Static CSS
    server.Get("/styles.css", [](const httplib::Request&, httplib::Response& res) {
        string css = readFile("styles.css");
        if (css.empty()) { res.status = 404; res.set_content("styles.css not found.", "text/plain"); return; }
        res.set_content(css, "text/css");
    });

    // Login GET
    server.Get("/login", [](const httplib::Request& req, httplib::Response& res) {
        if (getSession(req)) { res.set_redirect("/"); return; }
        res.set_content(buildLoginPage(""), "text/html");
    });

    // Login POST
    server.Post("/login", [](const httplib::Request& req, httplib::Response& res) {
        string studentName = getFormValue(req.body, "student_name");
        if (studentName.empty()) {
            res.set_content(buildLoginPage(buildMessage("Please enter your name to log in.", "error-message")), "text/html");
            return;
        }
        string sessionId = createSessionId();
        // FIX: construct the session in-place to avoid the deleted copy
        sessions[sessionId].studentName = studentName;
        res.set_header("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
        res.set_redirect("/");
    });

    // Logout POST
    server.Post("/logout", [](const httplib::Request& req, httplib::Response& res) {
        string sessionId = getCookieValue(req, "session_id");
        if (!sessionId.empty()) sessions.erase(sessionId);
        res.set_header("Set-Cookie", "session_id=deleted; Path=/; Max-Age=0");
        res.set_redirect("/login");
    });

    // Home GET
    server.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        StudentSession* session = getSession(req);
        if (!session) { redirectToLogin(res); return; }
        res.set_content(buildHomePage(*session, ""), "text/html");
    });

    // Expense GET
    server.Get("/expense", [](const httplib::Request& req, httplib::Response& res) {
        StudentSession* session = getSession(req);
        if (!session) { redirectToLogin(res); return; }
        res.set_content(buildExpensePage(*session, ""), "text/html");
    });

    // Add Expense POST
    server.Post("/add", [](const httplib::Request& req, httplib::Response& res) {
        StudentSession* session = getSession(req);
        if (!session) { redirectToLogin(res); return; }

        string category    = getFormValue(req.body, "category");
        string amountText  = getFormValue(req.body, "amount");
        string description = getFormValue(req.body, "desc");

        if (category.empty() || amountText.empty() || description.empty()) {
            res.set_content(buildExpensePage(*session,
                buildMessage("Please fill in all expense fields.", "error-message")), "text/html");
            return;
        }

        double amount = 0;
        try {
            amount = stod(amountText);
        } catch (...) {
            res.set_content(buildExpensePage(*session,
                buildMessage("Please enter a valid amount.", "error-message")), "text/html");
            return;
        }

        // FIX: negative amounts should be rejected with a user-visible error
        if (amount < 0) {
            res.set_content(buildExpensePage(*session,
                buildMessage("Amount cannot be negative.", "error-message")), "text/html");
            return;
        }

        Expense* expense = createExpense(category, amount, description);
        if (!expense) {
            res.set_content(buildExpensePage(*session,
                buildMessage("Unknown category selected.", "error-message")), "text/html");
            return;
        }

        session->tracker.addExpense(expense);
        res.set_redirect("/history");
    });

    // History GET
    server.Get("/history", [](const httplib::Request& req, httplib::Response& res) {
        StudentSession* session = getSession(req);
        if (!session) { redirectToLogin(res); return; }
        res.set_content(buildHistoryPage(*session, ""), "text/html");
    });

    // Delete expense POST
    server.Post("/delete", [](const httplib::Request& req, httplib::Response& res) {
        StudentSession* session = getSession(req);
        if (!session) { redirectToLogin(res); return; }

        string indexText = getFormValue(req.body, "index");
        if (!indexText.empty()) {
            try {
                // FIX: stoi result is 0-based; already correct but was
                //      previously unchecked for out-of-range (removeExpense
                //      guards this, but the try/catch is still good practice).
                session->tracker.removeExpense(stoi(indexText));
            } catch (...) {
                // ignore malformed index
            }
        }
        res.set_redirect("/history");
    });

    // Bill GET
    server.Get("/bill", [](const httplib::Request& req, httplib::Response& res) {
        StudentSession* session = getSession(req);
        if (!session) { redirectToLogin(res); return; }
        res.set_content(buildBillPage(*session, ""), "text/html");
    });

    cout << "Student Expense Tracker running at http://localhost:8080" << endl;
    server.listen("0.0.0.0", 8080);
    return 0;
}
