#include <iostream>
#include <queue>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <cstring>
#include <tuple>
#include <chrono>
#include <omp.h>
#include "sqlite3/sqlite3.h"
#include "trie.hpp"
#include "json.hpp"

typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

using namespace std;
using json = nlohmann::json;

int num_threads = 8;

struct page {
  int id;
  string title;
  page* parent;
};

void get_page_data(vector<sqlite3_stmt*> get_page_v, string title, int* page_id, string& page_title, sqlite3_destructor_type destructor = SQLITE_TRANSIENT) {
  sqlite3_stmt* get_page = get_page_v[omp_get_thread_num()];

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

vector<string> get_child_titles(vector<sqlite3_stmt*> get_links_v, int page_id) {
  sqlite3_stmt* get_links = get_links_v[omp_get_thread_num()];

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

int main(int argc, char* argv[]) {
	sqlite3* db_page;
	sqlite3* db_pagelinks;
	int rc_page;
	int rc_pagelinks;

  if (argc < 3) {
    json output;
    output["status"] = 500;
    output["data"]["msg"] = "Server error";
    cout << output.dump() << endl;
    return 1;
  }

	rc_page = sqlite3_open(argv[1], &db_page);
	rc_pagelinks = sqlite3_open(argv[2], &db_pagelinks);

	// attempt to open
	if (rc_page) {
    json output;
    output["status"] = 500;
    string msg =  "Can't open database: ";
    msg.append(sqlite3_errmsg(db_pagelinks));
    output["data"]["msg"] = msg;
    cout << output.dump() << endl;
		return 1;
	} else if (rc_pagelinks) {
    json output;
    output["status"] = 500;
    string msg =  "Can't open database: ";
    msg.append(sqlite3_errmsg(db_pagelinks));
    output["data"]["msg"] = msg;
    cout << output.dump() << endl;
		return 1;
	}

  string command = argv[3];
  transform(command.begin(), command.end(), command.begin(),
    [](unsigned char c){ return tolower(c); }); // convert command to lower case

	if (argc != 6 || !(command == "children" || command == "path")) {
    json output;
    output["status"] = 500;
    output["data"]["msg"] = "Server error";
    cout << output.dump() << endl;
		return 2;
	}

  vector<sqlite3_stmt*> get_page;
  vector<sqlite3_stmt*> get_links;

  for (int i = 0; i < num_threads; i++) {
    sqlite3_stmt* temp_get_page;
    sqlite3_stmt* temp_get_links;

    sqlite3_prepare_v2(db_page, "SELECT page_id,page_title FROM page WHERE page_title=?1", -1, &temp_get_page, 0);
    sqlite3_prepare_v2(db_pagelinks, "SELECT pl_title FROM pagelinks WHERE pl_from=?", -1, &temp_get_links, 0);

    get_page.push_back(temp_get_page);
    get_links.push_back(temp_get_links);
  }

  page* source = new page();
  get_page_data(get_page, argv[4], &(source->id), source->title, SQLITE_STATIC);

	if (source->id == -1) {
    json output;
    output["status"] = 400;
    output["data"]["msg"] = "Source page does not exist.";
    cout << output.dump() << endl;
		return 2;
	}

  source->parent = NULL;

  if (command == "children") {
    vector<string> children = get_child_titles(get_links, source->id);

    json j;
    int len = children.size();
    int level = atoi(argv[5]) + 1;

    #pragma omp parallel for num_threads(num_threads)
    for (int i = 0; i < len; i++) {
      int id;
      string ignored;

      get_page_data(get_page, children[i], &id, ignored);
      if (id != -1) {
        j["nodes"].push_back({{ "id", children[i] }, { "group", level }});
        j["links"].push_back({{ "source", source->title }, { "target", children[i] }});
      }
    }

    json output;
    output["status"] = 200;
    output["data"] = j;
    cout << output.dump() << endl;
    return 0;
  }

  string target = argv[5];

  int target_id;
  get_page_data(get_page, target, &target_id, target, SQLITE_STATIC);
	if (target_id == -1) {
    json output;
    output["status"] = 400;
    output["data"]["msg"] = "Target page does not exist.";
    cout << output.dump() << endl;
		return 2;
	}

  // cout << source->title << " -> " << target << endl;

  Trie t;
  queue<page*> q;
  
  t.insert(source->title);
  q.push(source);

  // begin timing
  auto start = std::chrono::system_clock::now();

  while (!q.empty()) {
    page* v = q.front();
    q.pop();
    if (v->title.compare(target) == 0) {
      //stop timing  and display runtime
  	  auto end = std::chrono::system_clock::now();
  	  auto elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  	  // std::cout << "\nruntime:" << elapsed_seconds.count() << " ms\n\n";

      page* current = v;

      forward_list<page*> path;

      while (current != NULL) {
        path.push_front(current);
        current = current->parent;
      }

      json j;

      unordered_map<string, int> groups;
      int group = 1;
      for (auto it = path.begin(); it != path.end(); ++it) {
        string title = (*it)->title;
        j["path"].push_back(title);
        
        if (groups.find(title) == groups.end()) {
          groups[title] = group;
        }

        group++;
        vector<string> children = get_child_titles(get_links, (*it)->id);
        int len = children.size();

        #pragma omp parallel for num_threads(num_threads)
        for (int i = 0; i < len; i++) {
          int id;
          string ignored;

          get_page_data(get_page, children[i], &id, ignored);
          if (id != -1) {
            j["links"].push_back({{ "source", (*it)->title }, { "target", children[i] }});
            if (groups.find(children[i]) == groups.end()) {
              groups[children[i]] = group;
            }
          }
        }
      }

      for (auto it = groups.begin(); it != groups.end(); ++it) {
        j["nodes"].push_back({{ "id", it->first }, { "group", it->second }});
      }

      json output;
      output["status"] = 200;
      output["data"] = j;
      cout << output.dump() << endl;
      return 0;
    }

    vector<string> children = get_child_titles(get_links, v->id);
    int len = children.size();

    #pragma omp parallel for num_threads(num_threads)
    for (int i = 0; i < len; i++) {
      string child_title = children[i];
      if (!t.search(child_title)) {
        t.insert(child_title);

        page* child = new page();
        get_page_data(get_page, child_title, &(child->id), child->title);

        if (child->id != -1) {
          child->parent = v;
          q.push(child);
        }
      }
    }
  }

  json output;
  output["status"] = 500;
  output["data"]["msg"] = "Could not find a route to the target node.";
  cout << output.dump() << endl;
  return 0;
}
