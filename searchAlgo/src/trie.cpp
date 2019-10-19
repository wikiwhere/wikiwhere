#include <iostream>
#include <vector>
#include "trie.hpp"

using namespace std;

vector<int> Trie::strip(string title) {
  vector<int> stripped;

  for (int i = 0, len = title.length(); i < len; i++) {
    int index = -1;
    int char_code = (int) title[i];

    if (char_code >= (int) 'a' && char_code <= (int) 'z') {
      index = char_code - (int) 'a';
    } else if (char_code >= (int) '0' && char_code <= (int) '9') {
      index = char_code - (int) '0' + 26;
    }

    if (index != -1) {
      stripped.push_back(index);
    }
  }

  return stripped;
}

Trie::Trie(void) {
  head = new node();
  head->is_end = false;
}

void Trie::insert(string title) {
  node *current = head;
  vector<int> indices = strip(title);

  for (int i = 0, len = indices.size(); i < len; i++) {
    int index = indices[i];
    if (current->children[index] == NULL) {
      current->children[index] = new node();
    }
    current = current->children[index];
  }
  current->is_end = true;
}

bool Trie::search(string title) {
  node *current = head;
  vector<int> indices = strip(title);

  for (int i = 0, len = indices.size(); i < len; i++) {
    int index = indices[i];
    if (current->children[index] == NULL) {
      return false;
    }
    current = current->children[index];
  }
  return current->is_end;
}
