//in the src folder
//#pragma once
#ifndef LCA_FINDER
#define LCA_FINDER
#include<vector>
#include<random>
#include<iostream>
#include<functional>
//#define MAXN (1<<17)
using namespace std;

inline mt19937 rg_lca_mine(897123765);
struct LCA{
    struct Treap{
        struct Node{
            Node *l;
            Node *r;
            int ver, lvl;
            int min_ver, min_lvl;
            int sz, pr;
            Node(): l(nullptr), r(nullptr){}
            Node(int _ver, int _lvl): ver(_ver), lvl(_lvl), min_ver(_ver), min_lvl(_lvl),
                                    l(nullptr), r(nullptr), sz(1), pr(rg_lca_mine()){}
            void con(){
                sz = 1;
                min_ver = ver;
                min_lvl = lvl;
                if (l){
                    sz += l->sz;
                    if (l->min_lvl < min_lvl){
                        min_lvl = l->min_lvl;
                        min_ver = l->min_ver;
                    }
                }
                if (r){
                    sz += r->sz;
                    if (r->min_lvl < min_lvl){
                        min_lvl = r->min_lvl;
                        min_ver = r->min_ver;
                    }
                }
            }
        };

        Node *root;
        Treap(): root(nullptr){}

        void Split_sz(Node *T, Node *&L, Node *&R, int sz){
            if (!T){
                L = R = nullptr;
                return;
            }
            int cur_sz = 1;
            if (T->l) cur_sz += T->l->sz;
            if (cur_sz > sz){
                R = T;
                Split_sz(T->l, L, R->l, sz);
            }
            else{
                L = T;
                Split_sz(T->r, L->r, R, sz - cur_sz);
            }
            T->con();
        }

        void Merge(Node *&T, Node *L, Node *R){
            if (!L){
                T = R;
                return;
            }
            if (!R){
                T = L;
                return;
            }
            if (L->pr > R->pr){
                T = L;
                Merge(T->r, L->r, R);
            }
            else{
                T = R;
                Merge(T->l, L, R->l);
            }
            T->con();
        }

        void Attach(int ver, int lvl){
            Node *cur = new Node(ver, lvl);
            Merge(root, root, cur);
        }

        void heapify(Node *T){
            Node *max = T;
            if (T->l && max->pr < T->l->pr) max = T->l;
            if (T->r && max->pr < T->r->pr) max = T->r;
            if (max != T){
                T->pr ^= max->pr ^= T->pr ^= max->pr;
                heapify(max);
            }
        }

        Node* Build(const vector<int>& ver, const vector<int>& height, int l, int r){
            if (l > r) return nullptr;
            int mid = (r + l) / 2;
            Node *T = new Node(ver[mid], height[ver[mid]]);
            T->l = Build(ver, height, l, mid - 1);
            T->r = Build(ver, height, mid + 1, r);
            heapify(T);
            T->con();
            return T;
        }

        int LCA(int l, int r){
            if (l > r) l ^= r ^= l ^= r;
            Node *L = nullptr, *R = nullptr, *Ans = nullptr;
            Split_sz(root, L, R, r + 1);
            Split_sz(L, L, Ans, l);
            int res = Ans->min_ver;
            Merge(L, L, Ans);
            Merge(root, L, R);
            return res;
        }
    };

    Treap T;
    vector<int> first;

    void init(vector<vector<int>> &adj, int root = 0){
        int n = adj.size();
        first.resize(n + 1);
        vector<int> height(n + 1);
        vector<bool> visited(n + 1, false);
        vector<int> order;
        function<void(int, int)> dfs = [&](int node, int h){
            visited[node] = true;
            height[node] = h;
            first[node] = order.size();
            order.push_back(node);
            for (auto to : adj[node]){
                if (!visited[to]){
                    dfs(to, h + 1);
                    order.push_back(node);
                }
            }
        };
        dfs(root, 0);
        T.root = T.Build(order, height, 0, order.size() - 1);
    }

    int lca(const int u, const int v){
        int l = first[u], r = first[v];
        if (l > r)
            swap(l, r);
        return T.LCA(l, r);
    }
};
#endif
