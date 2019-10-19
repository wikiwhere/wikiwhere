#include <iostream>
#include <queue>
#include <vector>
#include <cstring>
#include <tuple>
#include "sqlite3/sqlite3.h"
#include "trie.hpp"

typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

using namespace std;

struct page {
  int id;
  string title;
  page* parent;
};

void get_page_data(sqlite3_stmt* get_page, string title, int* page_id, string& page_title, sqlite3_destructor_type destructor = SQLITE_TRANSIENT) {
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

	if (page_id != 0) *page_id = sqlite3_column_int64(get_page, 0);
	page_title = temp_page_title;
}

vector<tuple<int, string>> get_page_links(sqlite3_stmt* get_page, sqlite3_stmt* get_links, int page_id) {
  vector<tuple<int, string>> child_ids;
	int rc;

  sqlite3_reset(get_links);
	sqlite3_bind_int(get_links, 1, page_id);

	do {
		rc = sqlite3_step(get_links);

		if (rc != SQLITE_ROW) break;

    int child_id;

		string link_title(reinterpret_cast<const char*>(sqlite3_column_text(get_links, 0)));
    string child_title;
    get_page_data(get_page, link_title, &child_id, child_title, SQLITE_STATIC);

    if (child_id != -1) child_ids.push_back(make_tuple (child_id, child_title));
	} while (true);

  return child_ids;
}

vector<string> get_child_titles(sqlite3_stmt* get_page, sqlite3_stmt* get_links, int page_id) {
  vector<string> child_titles;
	int rc;

  sqlite3_reset(get_links);
	sqlite3_bind_int(get_links, 1, page_id);

	do {
		rc = sqlite3_step(get_links);
		if (rc != SQLITE_ROW) break;

		string child_title(reinterpret_cast<const char*>(sqlite3_column_text(get_links, 0)));
    child_titles.push_back(child_title);
	} while (true);

  return child_titles;
}

vector<tuple<int, string>> get_children(sqlite3_stmt* get_page, sqlite3_stmt* get_links, string title, sqlite3_destructor_type destructor = SQLITE_TRANSIENT) {
	int page_id;
	string page_title;

	get_page_data(get_page, title, &page_id, page_title, destructor);

	cout << "id: " << page_id << " title: " << page_title << endl;

	return get_page_links(get_page, get_links, page_id);
}

int main(int argc, char* argv[]) {

	sqlite3* db_page;
	sqlite3* db_pagelinks;
	int rc_page;
	int rc_pagelinks;

	sqlite3_stmt* get_page;
	sqlite3_stmt* get_links;

	rc_page = sqlite3_open("../../testDb/enwikibooks-20191001-page.db", &db_page);
	rc_pagelinks = sqlite3_open("../../testDb/enwikibooks-20191001-pagelinks.db", &db_pagelinks);

	// attempt to open
	if (rc_page) {
		cout << "Can't open database: " << sqlite3_errmsg(db_page) << endl;
		return 1;
	} else if (rc_pagelinks) {
		cout << "Can't open database: " << sqlite3_errmsg(db_pagelinks) << endl;
		return 1;
	} else {
		cout << "Opened both databases successfully. Good" << endl;
	}
	
	if (argc != 3) {
		cout << "Wrong amount of parameters" << endl;
		return 2;
	}

	sqlite3_prepare_v2(db_page, "SELECT page_id,page_title FROM page WHERE page_title=?1", -1, &get_page, 0);
	sqlite3_prepare_v2(db_pagelinks, "SELECT pl_title FROM pagelinks WHERE pl_from=?", -1, &get_links, 0);

  page* source = new page();
  get_page_data(get_page, argv[1], &(source->id), source->title, SQLITE_STATIC);
  source->parent = NULL;
  string target = argv[2];

  cout << source->title << " -> " << target << endl;

  Trie t;
  queue<page*> q;
  
  t.insert(source->title);
  q.push(source);
  while (!q.empty()) {
    page* v = q.front();
    q.pop();
    // cout << v->title << " comp " << target << ": " << v->title.compare(target) << endl;
    if (v->title.compare(target) == 0) {
      page* current = v;
      while (current != NULL) {
        cout << current->id << " " << current->title << endl;
        current = current->parent;
      }
      return 0;
    }

    vector<string> children = get_child_titles(get_page, get_links, v->id);
    for (int i = 0, len = children.size(); i < len; i++) {
      string child_title = children[i];
      // cout << child_title << endl;
      if (!t.search(child_title)) {
        t.insert(child_title);

        page* child = new page();
        get_page_data(get_page, child_title, &(child->id), child->title);
        child->parent = v;
        q.push(child);
      }
    }
  }

  cout << "not found" << endl;

	return 0;
}
