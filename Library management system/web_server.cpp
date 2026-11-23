#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include "httplib.h"
#include "Model.h"
#include "Controller.h"

using namespace std;

LibraryDatabase g_db;
LibraryController g_ctrl(g_db);
mutex g_mtx;

string jsonEscape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}
