#include <iostream>
#include <queue>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <unordered_set>
#include "sqlite3/sqlite3.h"
#include "json.hpp"

typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

using namespace std;
using json = nlohmann::json;

struct page {
  int id;
  string title;
  page* parent;
};

void find_page_data(sqlite3_stmt* find_page, string title, int* page_id, string& page_title, sqlite3_destructor_type destructor = SQLITE_TRANSIENT) {
  sqlite3_reset(find_page);
  int rc = sqlite3_bind_text(find_page, 1, title.c_str(), -1, destructor);
  if (rc) {
    cout << "Bind failed " << rc << endl;
  }

  rc = sqlite3_step(find_page);
  if (rc != SQLITE_ROW) {
    if (page_id != 0) *page_id = -1;
    page_title = "";

    return;
  }

  string temp_page_title(reinterpret_cast<const char*>(sqlite3_column_text(find_page, 1)));

  if (page_id != 0) *page_id = sqlite3_column_int64(find_page, 0);
  page_title = temp_page_title;
}

void get_page_data(sqlite3_stmt* get_page, int id, int* page_id, string& page_title) {
  sqlite3_reset(get_page);
  int rc = sqlite3_bind_int(get_page, 1, id);
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

vector<int> get_child_ids(sqlite3_stmt* get_links, int page_id) {
  vector<int> child_ids;
  int rc;

  sqlite3_reset(get_links);
  sqlite3_bind_int(get_links, 1, page_id);

  do {
    rc = sqlite3_step(get_links);
    if (rc != SQLITE_ROW) break;

    int child_id = sqlite3_column_int64(get_links, 0);
    child_ids.push_back(child_id);
  } while (true);

  return child_ids;
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
    msg.append(sqlite3_errmsg(db_page));
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

  sqlite3_stmt* find_page;
  sqlite3_stmt* get_page;
  sqlite3_stmt* get_links;

  sqlite3_prepare_v2(db_page, "SELECT page_id,page_title FROM page WHERE page_namespace=0 AND page_title=?1", -1, &find_page, 0);
  sqlite3_prepare_v2(db_page, "SELECT page_id,page_title FROM page WHERE page_id=?1", -1, &get_page, 0);
  sqlite3_prepare_v2(db_pagelinks, "SELECT pl_to FROM pagelinks WHERE pl_from=?", -1, &get_links, 0);

  page* source = new page();
  find_page_data(find_page, argv[4], &(source->id), source->title, SQLITE_STATIC);

  if (source->id == -1) {
    json output;
    output["status"] = 400;
    output["data"]["msg"] = "Source page does not exist.";
    cout << output.dump() << endl;
    return 2;
  }

  source->parent = NULL;

  if (command == "children") {
    vector<int> children = get_child_ids(get_links, source->id);

    json j;
    int len = children.size();
    int level = atoi(argv[5]) + 1;

    j["nodes"].push_back({{ "id", source->title }, { "group", level - 1 }});
    for (int i = 0; i < len; i++) {
      int id;
      string title;

      get_page_data(get_page, children[i], &id, title);
      if (id != -1) {
        j["nodes"].push_back({{ "id", title }, { "group", level }});
        j["links"].push_back({{ "source", source->title }, { "target", title }});
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
  find_page_data(find_page, target, &target_id, target, SQLITE_STATIC);
  if (target_id == -1) {
    json output;
    output["status"] = 400;
    output["data"]["msg"] = "Target page does not exist.";
    cout << output.dump() << endl;
    return 2;
  }

  // cout << source->title << " -> " << target << endl;

  unordered_set<int> t;
  queue<page*> q;
  
  t.insert(source->id);
  q.push(source);

  while (!q.empty()) {
    page* v = q.front();
    q.pop();
    if (v->title.compare(target) == 0) {
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
        vector<int> children = get_child_ids(get_links, (*it)->id);
        int len = children.size();

        for (int i = 0; i < len; i++) {
          int id;
          string title;

          get_page_data(get_page, children[i], &id, title);
          if (id != -1) {
            j["links"].push_back({{ "source", (*it)->title }, { "target", title }});
            if (groups.find(title) == groups.end()) {
              groups[title] = group;
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

    vector<int> children = get_child_ids(get_links, v->id);
    int len = children.size();

    for (int i = 0; i < len; i++) {
      int child_id = children[i];
      if (t.find(child_id) != t.end()) {
        t.insert(child_id);

        page* child = new page();
        get_page_data(get_page, child_id, &(child->id), child->title);

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
