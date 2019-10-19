#include <iostream>
#include <vector>

using namespace std;

struct node {
  bool is_end;
  struct node *children[36];
  // allowed characters: a-z, 0-9
};

class Trie {
  private:
    node *head;
    vector<int> strip (string);
  public:
    Trie();
    void insert (string);
    bool search (string);
};
