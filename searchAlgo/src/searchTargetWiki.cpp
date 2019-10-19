#include <iostream>
#include <queue>
#include <vector>
#include "sqlite3/sqlite3.h"

typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

using namespace std;

void get_page_data(sqlite3_stmt* get_page, char* title, int* page_id, const unsigned char** page_title, sqlite3_destructor_type destructor = SQLITE_TRANSIENT) {
	if (sqlite3_bind_text(get_page, 1, title, -1, destructor)) {
		cout << "Bind failed" << endl;
	}

	sqlite3_step(get_page);
	*page_id = sqlite3_column_int64(get_page, 0);
	*page_title = sqlite3_column_text(get_page, 1);
}

void get_page_links(sqlite3_stmt* get_links, int page_id, vector<string>& link_titles) {

	sqlite3_bind_int(get_links, 1, page_id);

	int rc;

	do {
		rc = sqlite3_step(get_links);

		if (rc != SQLITE_ROW) break;

		const unsigned char* temp_link_title = sqlite3_column_text(get_links, 0);
		string link_title(reinterpret_cast<const char*>(temp_link_title));
		link_titles.push_back(link_title);
	} while (true);
}

int main(int argc, char* argv[]) {

	sqlite3* db_page;
	sqlite3* db_pagelinks;
	int rc_page;
	int rc_pagelinks;

	sqlite3_stmt* get_page;
	sqlite3_stmt* get_links;
	const char* tail;

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
	} else {
		cout << "From: " << argv[1] << endl;
		cout << "To: " << argv[2] << endl;
	}

	sqlite3_prepare_v2(db_page, "SELECT page_id,page_title FROM page WHERE page_title=?1", -1, &get_page, &tail);
	sqlite3_prepare_v2(db_pagelinks, "SELECT pl_title FROM pagelinks WHERE pl_from=?", -1, &get_links, &tail);

	cout << sqlite3_bind_parameter_count(get_page) << endl;

	int page_id;
	const unsigned char* page_title;

	get_page_data(get_page, argv[1], &page_id, &page_title, SQLITE_STATIC);

	cout << "Int: " << page_id << " title: " << page_title << endl;

	vector<string> link_titles;
	get_page_links(get_links, page_id, link_titles);
	cout << "s" << link_titles.size() << endl;
	for (int i = 0; i < link_titles.size(); i++) {
		cout << "Link: " << link_titles[i] << endl;
	}

	return 0;
}