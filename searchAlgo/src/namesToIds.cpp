#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <cstring>
#include <tuple>
#include <chrono>
#include <omp.h>
#include "sqlite3/sqlite3.h"

typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

using namespace std;

struct page {
  int id;
  string title;
  page* parent;
};

unordered_map<string, int> ids;

void get_page_data(sqlite3_stmt* get_page, string title, int* page_id, string& page_title, sqlite3_destructor_type destructor = SQLITE_TRANSIENT) {
  unordered_map<string, int>::const_iterator fetch = ids.find(title);
  if (fetch != ids.end()) {
    if (page_id != 0) *page_id = fetch->second;
    page_title = fetch->first;
    return;
  }

  sqlite3_reset(get_page);
  int rc = sqlite3_bind_text(get_page, 1, title.c_str(), -1, destructor);
  if (rc) {
    cout << "Bind failed " << rc << endl;
  }

  rc = sqlite3_step(get_page);
  if (rc != SQLITE_ROW) {
    if (page_id != 0) *page_id = -1;
    page_title = "";

    return;
  }

  string temp_page_title(reinterpret_cast<const char*>(sqlite3_column_text(get_page, 1)));

  int temp_page_id = sqlite3_column_int64(get_page, 0);
  if (page_id != 0) *page_id = temp_page_id;
  page_title = temp_page_title;
  pair<string, int> data (temp_page_title,temp_page_id);
  ids.insert(data);
}

int _main(int argc, char* argv[]) {
  sqlite3* db_page;
  int rc_page;

  rc_page = sqlite3_open(argv[1], &db_page);

  // attempt to open
  if (rc_page) {
    string msg =  "Can't open database: ";
    msg.append(sqlite3_errmsg(db_page));
    cout << msg << endl;
    return 1;
  }

  sqlite3_stmt* get_page;

  sqlite3_prepare_v2(db_page, "SELECT page_id,page_title FROM page WHERE page_namespace=0 AND page_title=?1", -1, &get_page, 0);

  ifstream filein(argv[2]);
  ofstream fileout(string(argv[2]) + ".o");

  if (!filein || !fileout) {
    cout << "Error opening files!" << endl;
    return 1;
  }

  string strTemp;
  while (filein >> strTemp) {
    size_t pos = 0;
    vector<string> parts;
    while ((pos = strTemp.find(",")) != string::npos) {
      parts.push_back(strTemp.substr(0, pos));
      strTemp.erase(0, pos + 1);
    }
    parts.push_back(strTemp);

    if (parts[1] != "0" || parts[3] != "0") {
      continue;
    }

    string pl_from = parts[0];
    string pl_title = parts[2];

    page* target = new page();
    get_page_data(get_page, pl_title, &(target->id), target->title);

    if (target->id == -1) {
      continue;
    }

    fileout << pl_from << "," << target->id << endl;

    delete target;
  }

  filein.close();
  fileout.close();
  return 0;
}
