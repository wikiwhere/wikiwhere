#include <iostream>
#include <queue>
#include <vector>
#include <list>
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
  page* next;
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

vector<page*> get_next_data(sqlite3_stmt* get_next, page* parent) {
  vector<page*> nextList;
  int rc;

  sqlite3_reset(get_next);
  sqlite3_bind_int(get_next, 1, parent->id);

  do {
    rc = sqlite3_step(get_next);
    if (rc != SQLITE_ROW) break;

    page* next = new page();

    next->title = string(reinterpret_cast<const char*>(sqlite3_column_text(get_next, 1)));
    next->id = sqlite3_column_int64(get_next, 0);
    next->next = parent;

    nextList.push_back(next);
  } while (true);

  return nextList;
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
  sqlite3_stmt* get_children;
  sqlite3_stmt* get_parents;

  sqlite3_exec(db_pagelinks, ("ATTACH '" + string(argv[1]) + "' as page;").c_str(), NULL, NULL, NULL);
  sqlite3_prepare_v2(db_pagelinks, "SELECT page_id,page_title FROM page.page WHERE page_namespace=0 AND page_title=?1", -1, &find_page, 0);
  sqlite3_prepare_v2(db_pagelinks, "SELECT page_id,page_title FROM page.page WHERE page_id=?1", -1, &get_page, 0);
  sqlite3_prepare_v2(db_pagelinks, "SELECT pl_to FROM main.pagelinks WHERE pl_from=?", -1, &get_links, 0);

  sqlite3_prepare_v2(db_pagelinks, "SELECT page_id,page_title FROM main.pagelinks a INNER JOIN page.page b ON b.page_id = a.pl_to WHERE pl_from=?", -1, &get_children, 0);
  sqlite3_prepare_v2(db_pagelinks, "SELECT page_id,page_title FROM main.pagelinks a INNER JOIN page.page b ON b.page_id = a.pl_from WHERE pl_to=?", -1, &get_parents, 0);

  page* source = new page();
  find_page_data(find_page, argv[4], &(source->id), source->title, SQLITE_STATIC);

  if (source->id == -1) {
    json output;
    output["status"] = 400;
    output["data"]["msg"] = "Source page does not exist.";
    cout << output.dump() << endl;
    return 2;
  }

  source->next = NULL;

  if (command == "children") {
    vector<page*> children = get_next_data(get_children, source);

    json j;
    int len = children.size();
    int level = atoi(argv[5]) + 1;

    j["nodes"].push_back({{ "id", source->title }, { "group", level - 1 }});
    for (int i = 0; i < len; i++) {
      int id = children[i]->id;
      string title = children[i]->title;

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

  page* target = new page();
  find_page_data(find_page, argv[5], &(target->id), target->title, SQLITE_STATIC);
  if (target->id == -1) {
    json output;
    output["status"] = 400;
    output["data"]["msg"] = "Target page does not exist.";
    cout << output.dump() << endl;
    return 2;
  }

  // cout << source->title << " -> " << target << endl;

  unordered_map<int, page*> tf;
  unordered_map<int, page*> tb;
  queue<page*> qf;
  queue<page*> qb;
  
  tf[source->id] = source;
  qf.push(source);
  
  tb[target->id] = target;
  qb.push(target);

  while (!qf.empty() && !qb.empty()) {
    queue<page*>* q = &qf;
    unordered_map<int, page*>* t = &tf;
    unordered_map<int, page*>* trev = &tb;
    sqlite3_stmt** get_next = &get_children;

    if (qf.size() > qb.size()) {
      q = &qb;
      t = &tb;
      trev = &tf;
      get_next = &get_parents;
    }

    page* v = q->front();
    q->pop();

    if (v->title == "") {
      get_page_data(get_page, v->id, &(v->id), v->title);
      if (v->id == -1) continue;
    }

    if (trev->find(v->id) != trev->end()) {
      page* current = v;
      page* currentrev = (*trev)[v->id];

      list<page*> path;

      while (current != NULL) {
        path.push_front(current);
        current = current->next;
      }
      while (currentrev != NULL) {
        path.push_back(currentrev);
        currentrev = currentrev->next;
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
        vector<page*> children = get_next_data(get_children, (*it));
        int len = children.size();

        for (int i = 0; i < len; i++) {
          int id = children[i]->id;
          string title = children[i]->title;

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

    vector<page*> next = get_next_data(*get_next, v);
    int len = next.size();

    for (int i = 0; i < len; i++) {
      int next_id = next[i]->id;
      if (t->find(next_id) == t->end()) {
        (*t)[next_id] = next[i];

        q->push(next[i]);
      }
    }
  }

  json output;
  output["status"] = 500;
  output["data"]["msg"] = "Could not find a route to the target node.";
  cout << output.dump() << endl;
  return 0;
}
