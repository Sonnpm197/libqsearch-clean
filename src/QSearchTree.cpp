#include "QSearchTree.hpp"
#include <cmath> 
#include <cassert>
#include <algorithm>

#include "RandTools.hpp"
#include "QSearchNeighborList.hpp"
#include "QSearchFullTree.hpp"
#include "SimpleMatrix.hpp"
#include "QSearchConnectedNode.hpp"

QSearchTree::QSearchTree(QMatrix<double>& dm_init) 
  : dm( dm_init), 
    total_node_count(dm_init.dim * 2 - 2), 
    nodeflags(dm_init.dim * 2 - 2, 0),
    n(dm_init.dim * 2 - 2), 
    dist_calculated(false), 
    must_recalculate_paths(true), 
    spm(dm_init.dim * 2 - 2),
    score(0.0),
    dist_min(0.0), 
    dist_max(0.0),
    f_score_good(false)
{
  assert(dm.dim >= 4);

  for(int i = 0; i < dm.dim - 2; i++ ) {
    connect(i, dm.dim + i);
    if (i > 0)
      connect(i + dm.dim-1, i + dm.dim);
  }
  connect(dm.dim - 2, dm.dim);
  connect(dm.dim-1, total_node_count-1);

  for(int i = 0; i < total_node_count; i += 1) {
    if (get_neighbor_count(i) == 1) {
      leaf_placement.push_back( i );
    }
  }
}

QSearchTree::QSearchTree(const QSearchTree& q) : 
  total_node_count(q.total_node_count), 
  must_recalculate_paths(true), 
  dist_calculated(false),
  score(q.score),
  spm(q.spm),
  f_score_good(false), 
  dist_min(0.0), 
  dist_max(0.0),
  ms(q.ms), 
  n(q.n),
  nodeflags(q.nodeflags),
  leaf_placement(q.leaf_placement),
  dm(q.dm) 
{  
  ms.total_clonings++; 
}

std::unique_ptr< QSearchTree > QSearchTree::find_better_tree(int howManyTries) 
{
    if (!dist_calculated) {
        calc_min_max();
        dist_calculated = 1;
    }
  
  //QSearchTree result;
  int i, totmuts;
  double candscore;
  double best_score;
  assert( this );
  double curscore = score_tree();
  std::unique_ptr< QSearchTree > cand, result;
#if QSOPENMP_ENABLED
  if (!g_thread_supported ()) g_thread_init (NULL);
#pragma omp parallel shared(result, curscore) private(i, totmuts, cand, candscore, best_score) firstprivate(howManyTries, dm, clt)
 {
#pragma omp for schedule (dynamic, 1)
#else
  howManyTries = 1;
  do {
#endif
  for (i = 0; i < howManyTries; i += 1) {
    cand.reset( new QSearchTree( *this ) );
     
    //qsearch_tree_complex_mutation(cand);
    QSearchFullTree tree(*cand);

    // perform node_count swaps, keep track of best
    best_score = tree.raw_score;
    totmuts = tree.node_count;//qsearch_tree_get_mutation_distribution_sample(clt);
    
    int j;
     
    for (j = 0; j < totmuts; ++j) {
        double beta = 1.0; // set to 0.0 to mimick random behaviour. This behaviour is a metropolis markov chain
         
        unsigned int p1, p2;
        tree.random_pair(p1, p2);

        double cur = tree.raw_score;

        if ( rand_int(0, 2) < 2) { 
            
            tree.swap_nodes(p1, p2);
            
            if (tree.raw_score <= best_score || fabs(tree.raw_score - best_score) < 1e-6) { 
                cand = tree.to_searchtree();
                best_score = tree.raw_score;
                //printf("Score improved from %f to %f, raw: %f \n", curscore, cand->score, tree.raw_score);
            }

            // calculate acceptance
            double now = tree.raw_score;
            if (rand1(gen) >= exp(beta * (cur-now) )) { // reject
                tree.swap_nodes(p1, p2);
            } 

        } else { // transfer tree
            
            int interior = tree.move_to(p1, p2);
            assert(interior != p2);
             
            int sibling = tree.find_sibling(p1, p2);
           
            // move entire subtree containing p1 and sibling in the place of p2 
            tree.swap_nodes(interior, p2);
            
            if (tree.raw_score <= best_score || fabs(tree.raw_score - best_score) < 1e-6) { 
                cand = tree.to_searchtree();
                best_score = tree.raw_score;
                //printf("Score improved from %f to %f, raw: %f \n", curscore, cand->score, tree.raw_score);
            }
            
            // swap the sibling back in its original place (making 'node' a sibling of 'p2'
            tree.swap_nodes(sibling, p2);
           
            // postcondition: 
            assert( tree.find_sibling(p1, sibling) == p2);

            if (tree.raw_score <= best_score || fabs(tree.raw_score - best_score) < 1e-6) { 
                cand = tree.to_searchtree();
                best_score = tree.raw_score;
                //printf("Score improved from %f to %f, raw: %f \n", curscore, cand->score, tree.raw_score);
            }
            
            // calculate acceptance
            double now = tree.raw_score;
            if (rand1(gen) >= exp(beta*(cur-now) )) { // reject
                tree.swap_nodes(sibling, p2);
                tree.swap_nodes(interior, p2);
            } 
        }
    }
    
    //tree.to_searchtree(cand);
    //qsearch_free_fulltree(tree);
    
    //cand->f_score_good = 0;
    //double org = cand->score;
    assert( cand.get() != NULL);
    candscore  = cand->score_tree();
    
    //if (fabs(org-cand->score) > 1e-6) {
    //    printf("Error, score should be %f, was %f\n", cand->score, org);
    //    exit(0);
    //}

    if (candscore <= curscore) {
      cand.reset();
      continue;
    }
#if QSOPENMP_ENABLED
#pragma omp critical
#endif
    if (candscore > curscore) {
      if (result)
      result.reset();
      std::swap(result,cand);
      curscore = candscore;
      //std::cout << "Got better score " << curscore << std::endl;
    }
    else
      cand.reset();
    }
#if QSOPENMP_ENABLED
}
#else
  } while (0);
#endif
  return result;
}

unsigned int QSearchTree::get_leaf_node_count()
{
  return (total_node_count+2)/2;
}

unsigned int QSearchTree::get_kernel_node_count()
{
  return (total_node_count-2)/2;
}

// possibly return unique pointer? call by reference?
QMatrix< unsigned int> QSearchTree::get_adjacency_matrix()
{
  QMatrix<unsigned int> m(total_node_count);
  int i, j;
  for (i = 0; i < total_node_count; i++) {
    for (j = i+1; j < total_node_count; j++) {
      assert( j < total_node_count );
      int b = is_connected( i, j) ? 1 : 0;
      m[i][j] = b;
      m[j][i] = b;
    }
  }
  return m;
}

void QSearchTree::calc_min_max() {
  //std::cout << "\nQSearchTree::calc_min_max()\n";
  //std::fflush( stdout );

  dist_min = 0.0;
  dist_max = 0.0;
  for (int i = 0; i < leaf_placement.size(); i += 1)
      for (int j = i+1; j < leaf_placement.size(); j += 1)
          for (int k = j+1; k < leaf_placement.size(); k += 1)
              for (int l = k+1; l < leaf_placement.size(); l += 1) {
                  double c1, c2, c3;
                  c1 = dm[i][j] + dm[k][l];
                  c2 = dm[i][k] + dm[j][l];
                  c3 = dm[i][l] + dm[j][k];

                  dist_min += std::min( { c1, c2, c3 } ); dist_max += std::max( { c1, c2, c3 } );
              }
  // wheee!!
  //std::cout << "\nQSearchTree::calc_min_max() complete\n";
  //std::fflush( stdout );

}

bool QSearchTree::is_connected(const unsigned int& a, const unsigned int& b) 
{
  // std::cout << "QSearchTree::is_connected() - a = " << a << " b = " << b << "\n";
  assert(a >= 0 && b >= 0 && a < total_node_count && b < total_node_count);
  if (a == b) return false;
  return a > b ?  n[b].has_neighbor(a) : n[a].has_neighbor(b); // backwards? fixed
}

bool QSearchTree::is_standard_tree()
{
  for (unsigned int i = 0; i < total_node_count; i++) {
    unsigned int nc = get_neighbor_count(i);
    if (nc != 1 && nc != 3)
      return false;
  }
  return true;
}

unsigned int QSearchTree::get_neighbor_count(const unsigned int& a) {
  int acc = 0;
  assert( a < total_node_count );
  for (unsigned int i = 0; i < total_node_count; i++)
    if (is_connected(i, a))
      acc += 1;
  return acc;
}

void QSearchTree::connect(const unsigned int& a, const unsigned int& b)
{
  //std::cout << "QSearchTree::connect() - a = " << a << " b = " << b << "\n";
  assert( a < total_node_count );
  assert( b < total_node_count );
  assert(is_connected(a,b) == false);
  assert(a != b);
  if (a < b)
    n[a].add_neighbor(b);
  else
    n[b].add_neighbor(a);
  must_recalculate_paths = true;
  f_score_good = false;
}

void QSearchTree::disconnect(const unsigned int& a, const unsigned int& b)
{
  assert(is_connected(a,b) == true);
  assert(a != b);
  if (a < b)
    n[a].remove_neighbor(b);
  else
    n[b].remove_neighbor(a);
  must_recalculate_paths = true;
  f_score_good = false;
}

// changed to call by reference
void QSearchTree::find_path(NodeList& result, unsigned int a, unsigned int b) {
  find_path_fast(result, a, b);
}

// changed argument order
void QSearchTree::find_path_fast(NodeList& result, unsigned int a, unsigned int b)
{
  result.clear();
  assert(a >= 0 && b >= 0 && a < total_node_count && b < total_node_count);
  freshen_spm();
  
  int step_counter = -1;
  for (;;) {    
    result.push_back(a);
    step_counter += 1;
    if (step_counter > total_node_count)
      break;
    if (a == b)
      break;
    a = spm[b][a];
  }
  //std::cout << "QSearchTree::find_path_fast() - result ";
  //for(auto r : result) std::cout << r << " ";
  //std::cout <<  "\n";
  if (a != b)
    std::cout << "Error, broken path from " << a << " to " << b << " for tree.\n";
}

unsigned int QSearchTree::find_path_length(unsigned int& a, unsigned int& b)
{
  find_path_fast(p1, a, b);
  return p1.size();
}

void QSearchTree::freshen_spm()
{
  //guint32 target;
  if (!must_recalculate_paths)
    return;
  must_recalculate_paths = 0;
  assert(total_node_count > 1);
  //for (target = 0; target < total_node_count; target += 1)
  //  qsearch_calculate_spm_for(clt, nodeflags, target);

  QSearchConnectedNodeMap map(*this);
    
  int i,j;
  for (i=0;i<total_node_count;++i) {  
      auto& spm_connect = spm[i];
      for (j=0;j<total_node_count; ++j) {
         if (j==i) continue;
         // path from j to i
         spm_connect[j] = map[j].connections[ (int) map[j].node_branch[i] ]; 
      }
  }
}

bool QSearchTree::is_consistent_quartet(unsigned int &a, unsigned int &b, unsigned int &c, unsigned int &d)
{
  //std::cout << "QSearchTree::is_consistent_quartet()\n";
  assert( a < total_node_count );
  assert( b < total_node_count );
  assert( c < total_node_count );
  assert( d < total_node_count );

  assert(get_neighbor_count(a) == 1);
  assert(get_neighbor_count(b) == 1);
  assert(get_neighbor_count(c) == 1);
  assert(get_neighbor_count(d) == 1);
  
  for( auto& flag : nodeflags) flag &= ~NODE_FLAG_QUARTETINT;
  find_path_fast(p1, a, b);
  for( auto node : p1 ) nodeflags[node] |= NODE_FLAG_QUARTETINT;
  find_path_fast(p2, c, d);
  for( auto node : p2 ) if( nodeflags[node] & NODE_FLAG_QUARTETINT) {
    // std::cout << "QSearchTree::is_consistent_quartet() false - collision at node #" << node << "\n";
    return false;
  }
  return true;
}

unsigned int QSearchTree::get_random_node(const node_type& what_kind)
{
  unsigned int result;
  unsigned int n;
  //std::cout << "QSearchTree::get_random_node()\n";
  assert(what_kind == NODE_TYPE_LEAF || what_kind == NODE_TYPE_KERNEL ||
           what_kind == NODE_TYPE_ALL);
  do {
    result = rand_int(0, total_node_count - 1);
    //std::cout << "result = " << result;
    n = get_neighbor_count(result);
    //std::cout << " n = " << n << "\n";
  } while ((what_kind & (n == 1 ? NODE_TYPE_LEAF : NODE_TYPE_KERNEL)) == 0);
  return result;
}

unsigned int QSearchTree::get_random_node_but_not(const node_type& what_kind, const unsigned int& but_not)
{
  //std::cout << "QSearchTree::get_random_node_but_not() but_not = " << but_not << "\n";
  unsigned int result;
  do {
    result = get_random_node(what_kind);
    //std::cout << "result = " << result << "\n";
  } while (result == but_not);
  return result;
}

unsigned int QSearchTree::get_random_neighbor(const unsigned int& who)
{
  unsigned int result;
  NodeList neighbors;
  get_neighbors(neighbors, who);
  result = neighbors[ rand_int( 0, neighbors.size() - 1 ) ];
  return result;
}

void QSearchTree::get_neighbors(NodeList& neighbors, const unsigned int &who) {
  neighbors.clear();
  for (int i = 0; i < total_node_count; i++) 
    if (is_connected(i, who)) neighbors.push_back(i);
}

bool QSearchTree::is_valid_tree()
{
  unsigned int i, j;
  assert(total_node_count > 3);
  for (i = 0; i < leaf_placement.size(); i++) 
    { if( get_neighbor_count(leaf_placement[i] != 1 ) ) return false; }

  for (i = 0; i < total_node_count; i += 1) {
    int nc = get_neighbor_count(i);
    if (nc != 1 && nc != 3)
      return false;
  }

  for (i = 0; i < total_node_count; i++)
    for (j = 0; j < total_node_count; j += 1) {
      int pl1 = find_path_length(i, j);
      int pl2 = find_path_length(j, i);
      if (pl1 != pl2)
        return false;
      if (pl1 < 1 || pl1 > total_node_count)
        return false;
    }
  return true;
}

void QSearchTree::complex_mutation()
{
  ms.last_simple_mutations = 0;
  int totmuts = get_mutation_distribution_sample();
  for (int i = 0; i < totmuts; i += 1)
    simple_mutation();
  ms.total_simple_mutations += ms.last_simple_mutations;
  ms.total_complex_mutations += 1;
}

int QSearchTree::get_mutation_distribution_sample()
{
  const int MAXMUT = 80;
  std::vector<int> p;

  for (int i = 0; i < MAXMUT; i++) {
    double k = i + 4; /* to make single-mutations somewhat less common */
    p.push_back((int)(1000000.0 / (k * (log(k) / log(2.0)) * (log(k)/log(2.0)))));
  }
  std::default_random_engine generator;
  std::discrete_distribution<> d(p.begin(),p.end());
  return d(generator)+1;
}

void QSearchTree::simple_mutation()
{
  bool hm = false;
  int i;
  do {
    i = rand_int(0,2);
    //std::cout << "simple mutation type " << i << "\n";
    switch (i) {
      case 0: simple_mutation_leaf_swap(); hm = true; break;
      case 1: if (can_subtree_transfer()) { simple_mutation_subtree_transfer(); hm = true; } break;
      case 2: if (can_subtree_interchange()) { simple_mutation_subtree_interchange(); hm = true; } break;
    }
  } while (!hm);
}

void QSearchTree::simple_mutation_leaf_swap()
{
  unsigned int l1, l2;
  l1 = get_random_node(NODE_TYPE_LEAF);
  l2 = get_random_node_but_not(NODE_TYPE_LEAF, l1);
  std::swap( leaf_placement[l1], leaf_placement[l2] );
  f_score_good = false;
  ms.last_simple_mutations += 1;
}

void QSearchTree::simple_mutation_subtree_transfer()
{
  std::cout << "QSearchTree::simple_mutation_subtree_transfer()\n";
  assert(can_subtree_transfer());
  unsigned int k1, k2, i1, m1, m2, m3;
  do {
    k1 = get_random_node(NODE_TYPE_ALL);
    k2 = get_random_node_but_not(NODE_TYPE_KERNEL, k1);
  } while (find_path_length(k1, k2) <= 2);
  NodeList path, neighbors; 
  find_path(path, k1, k2);
  i1 = path[1];
  disconnect(k1, i1);
  get_neighbors(neighbors, i1);
  do {
    m3 = get_random_neighbor(k2);
  } while (m3 == path[path.size()-2]);
  m1 = neighbors[0];
  m2 = neighbors[1];
  disconnect(m1, i1);
  disconnect(m2, i1);
  disconnect(m3, k2);
  connect(m1, m2);
  connect(k2, i1);
  connect(m3, i1);
  connect(k1, i1);
  ms.last_simple_mutations += 1;
}

void QSearchTree::simple_mutation_subtree_interchange()
{
  unsigned int k1, k2, n1, n2;
  assert(can_subtree_interchange());
  do {
    k1 = get_random_node(NODE_TYPE_KERNEL);
    k2 = get_random_node_but_not(NODE_TYPE_KERNEL, k1);
  } while (find_path_length(k1, k2) <= 3);
  NodeList path; 
  find_path(path, k1, k2);
  n1 = path[1];
  n2 = path[path.size()-2];
  disconnect(n1, k1);
  disconnect(n2, k2);
  connect(n1, k2);
  connect(n2, k1);
  ms.last_simple_mutations += 1;
}

bool QSearchTree::can_subtree_transfer()
{
  return (total_node_count >= 9);
}

bool QSearchTree::can_subtree_interchange()
{
  return (total_node_count >= 11);
}

void QSearchTree::walk_tree(NodeList& result, const unsigned int& fromwhere, bool f_bfs)
{
  result.clear();
  NodeList todo;
  unsigned int d = 0, s = total_node_count, v = fromwhere;
  todo.push_back(v);
  for (unsigned int i = 0; i < s; i += 1) nodeflags[i] &= ~NODE_FLAG_ISWALKED;
  while (d < s) {
    assert(todo.size() > 0);
    int remind = (f_bfs ? 0 : (todo.size()-1));
    unsigned int nextguy = todo[remind];
    todo.erase(todo.begin()+remind);
    result.push_back(nextguy);
    nodeflags[nextguy] |= NODE_FLAG_ISWALKED;
    d += 1;
    NodeList nlist;
    get_neighbors(nlist, nextguy);
    if (nlist.size() == 3 && ((nodeflags[nextguy] & NODE_FLAG_ISFLIPPED) != 0))
      { std::reverse(nlist.begin(), nlist.end()); }

    for (int i = 0; i < nlist.size(); i++) {
      if (nodeflags[nlist[i]] & NODE_FLAG_ISWALKED)
        continue;
      todo.push_back(v);
    }
  }
}

  void QSearchTree::walk_tree_bfs(NodeList& result, const unsigned int& fromwhere)
{
  walk_tree(result, fromwhere, true);
}

  void QSearchTree::walk_tree_dfs(NodeList& result, const unsigned int& fromwhere)
{
  walk_tree(result, fromwhere, false);
}

double QSearchTree::calculate_order_cost()
{
  int i;
  double acc = 0.0;
  NodeList res, bres; 
  flipped_node_order(bres);
  for (i = 0; i < bres.size(); i += 1) {
    unsigned int a;
    a = bres[i];
    if (get_neighbor_count(a) != 1)
      continue;
    a = get_column_number(a);
    res.push_back(a);
  }
  for (i = 0; i < res.size(); i += 1) {
    double c;
    unsigned int a = res[i];
    unsigned int b = res[(i+1)%(res.size())];
    c = dm[a][b] + dm[b][a];
    acc += c;
  }
  return acc;
}

unsigned int QSearchTree::get_column_number(const unsigned int& nodenum)
{
  for (unsigned int i = 0; i < leaf_placement.size(); i += 1)
    if (leaf_placement[i] == nodenum)
      return i;
  std::cout << "QSearchTree::get_column_number - Bad column number" << nodenum << "\n";
  return 0;
}

void QSearchTree::mutate_order_simple()
{
  int k = get_random_node(NODE_TYPE_KERNEL);
  nodeflags[k] ^= NODE_FLAG_ISFLIPPED;
  //  printf("made node %d flip with %d neighbors\n", k, get_neighbor_count(k));
  ms.last_order_simple_mutations += 1;
}

void QSearchTree::mutate_order_complex()
{
  ms.last_order_simple_mutations = 0;
  do {
    mutate_order_simple();
  } while (fair_coin());
  ms.total_order_simple_mutations += ms.last_order_simple_mutations;
  ms.total_order_complex_mutations += 1;
}

void QSearchTree::flipped_node_order(NodeList& nodes)  
{
  walk_tree_dfs(nodes, 0);
}

/* Returns true if every node has exactly 1 or 3 neighbors */
bool QSearchTree::is_tree_ternary()
{
  for (int i = 0; i < total_node_count; i++) {
    int nc = get_neighbor_count(i);
    if (nc != 1 && nc != 3)
      return false;
  }
  return true;
}

/* Sets the "connectedness" state (true or false) between nodes a and b and
 * returns the old connectedness status that was overwritten.
 */
bool QSearchTree::set_connected(const unsigned int& a, const unsigned int& b, bool newconstate)
{
  assert(a >= 0 && b >= 0 && a < total_node_count && b < total_node_count);
  if (a == b)
    return false;
  bool oldconstate = is_connected(b, a);
  if (oldconstate != newconstate) {
    if (newconstate)
      connect(a, b);
    else
      disconnect(a, b);
  }
  return oldconstate;
}

void QSearchTree::clear_all_connections()
{
  for (unsigned int i = 0; i < total_node_count; i += 1)
    for (unsigned int j = i+1; j < total_node_count; j += 1)
      set_connected(j, i, false);
}

// deferred
/*
gdouble QSearchTree::read_from_dot(GString *treedot, LabeledMatrix *lm)
{
  gchar **lines, *cur, *str;
  const gchar *scoreprefix = "label=\"S(T)=";
  lines = g_strsplit(treedot->str, "\n", 0);
  int i;
  double score;
  gboolean gotscore = false;
  clear_all_connections();
  for (i = 0; (cur = lines[i]) != NULL; i += 1) {
    unsigned int a, b;
    if (g_str_has_prefix(cur, scoreprefix)) {
      str = cur + strlen(scoreprefix);
      strtok(str, "\"");
      score = atof(str);
      gotscore = true;
      continue;
    }
    if (strlen(cur) < 3)
      continue;
    if (strstr(cur, "label") || strstr(cur, "dotted") || strstr(cur, "graph"))
      continue;
    sscanf(cur, "%d -- %d", &a, &b);
    connect(b, a);
  }
  for (i = 0; (cur = lines[i]) != NULL; i += 1)
    free(cur);
  free(lines);
  flatten_leafperm();
  if (is_tree_ternary())
    return gotscore ? score : -2.0;
  else
    return -1.0;
}
*/

double QSearchTree::score_tree()
{
  //std::cout << "\nQSearchTree::score_tree()\n";
  assert(this);
  if (!dist_calculated) {
    calc_min_max();
    dist_calculated = true;
  }
   
  double score2 = score_tree_fast_v2();
  
  double acc = score2;

  double ERRTOL = 1.0e-6;  // ERRTOL undefined in C version repository. 
  double amin = dist_min; 
  double amax=dist_max;
  //std::cout << "acc = " << acc << " amin = " << amin << " amax = " << amax << "\n";
  assert(amax >= amin - ERRTOL);
  assert(acc >= amin - ERRTOL);
  assert(acc <= amax + ERRTOL);
  score = (amax-acc)/(amax-amin);
  f_score_good = true;
  //std::cout << "QSearchTree::score_tree() - returning " << score << "\n";
  assert(score >= 0.0 - ERRTOL);
  assert(score <= 1.0 + ERRTOL);

  return score;
}

double QSearchTree::score_tree_original()
{
  if (!dist_calculated) {
    calc_min_max();
    dist_calculated = true;
  }

/*
  if (f_score_good) { 
    std::cout << "\nQSearchTree::score_tree() - score good, returning " << score << "\n";
    std::fflush( stdout );
    return score;
  }
*/

  unsigned int i, j, k, l;
  double acc = 0.0;
  double amin=0.0, amax=0.0;

  // assert matrix values are nonnegative
  //std::string s;
  //dm.to_string(s);
  //std::cout << s;
  //std::cout << "\nassert matrix values are nonnegative\n";
  //std::fflush( stdout );
  //for(auto& v : dm.m) for(auto& w : v) assert(w >= 0.0);  
    //freshen_spm();
    
    //struct timespec start_time;
    //struct timespec end_time;
    //clockid_t clockid = CLOCK_REALTIME; 
    //clock_gettime(clockid, &start_time);
    
#ifdef SKIP_ORIGINAL
if(0) // will skip loop
#endif

  unsigned int lps = leaf_placement.size();

  for (i = 0; i < lps; i += 1) {
    for (j = i+1; j < lps; j += 1) {
      for (k = j+1; k < lps; k += 1) {
        for (l = k+1; l < lps; l += 1) {
          unsigned int ni = leaf_placement[i];
          unsigned int nj = leaf_placement[j];
          unsigned int nk = leaf_placement[k];
          unsigned int nl = leaf_placement[l];
          bool x1, x2;
          double c1, c2, c3;
          c1  = dm[i][j] + dm[k][l];
          c2  = dm[i][k] + dm[j][l];
          c3  = dm[i][l] + dm[j][k];
          /*minscore = c1; maxscore = c1;
          if (c2 < minscore) minscore = c2;
          if (c3 < minscore) minscore = c3;
          if (c2 > maxscore) maxscore = c2;
          if (c3 > maxscore) maxscore = c3;
          amin += minscore; amax += maxscore;*/
          x1 = is_consistent_quartet(ni,nj,nk,nl);
          if (x1) { acc += c1; continue; }
          x2 = is_consistent_quartet(ni,nk,nj,nl);
          if (x2) { acc += c2; continue; }
#ifdef G_DISABLE_ASSERT
          acc += c3; continue;
#else
          bool x3 = is_consistent_quartet(ni,nl,nj,nk);
          if (x3)
            acc += c3;
          else {
            std::cout << "QSearchTree::score_tree() - Error in program logic: no consistent quartets for \n";
            std::cout << ni << " " << nj << " " << nk << " " << nl << " yielded " << x1 << " " << x2 << " " << x3 << "\n";
            std::ofstream f("treefile.dot");
            f << to_dot();
            f.close();
            assert(0); 
          }
#endif
        }
      }
    }
  }

  //long nanos_per_second = 1000000000L;
  //clock_gettime(clockid, &end_time);
  //int nanos1 = ((end_time.tv_sec - start_time.tv_sec) * nanos_per_second + end_time.tv_nsec - start_time.tv_nsec);
  //start_time = end_time;
    
   //qsearch_optimize_tree(dm);
   static int nEvals = 0;
   if (++nEvals % 10 == 0) { std::cout << "Evals: " << nEvals << "\n"; }
   
   double score2 = score_tree_fast_v2();
  //clock_gettime(clockid, &end_time);
  int inOrder = 0;

  if (inOrder) {  
      QMatrix<double> dm2(dm.dim);
      for (i=0; i < dm.dim; ++i) {
          for (j=0;j<dm.dim;++j) {
            dm2[leaf_placement[i]][leaf_placement[j]] = dm[i][j];
          }
      }
      QSearchFullTree tree(*this);
      tree.dm = dm2;
      printf("Raw scores %f %f\n", score2, tree.raw_score);
      exit(0);
  }
        
  //int nanos2 = ((end_time.tv_sec - start_time.tv_sec) * nanos_per_second + end_time.tv_nsec - start_time.tv_nsec);

#ifdef SKIP_ORIGINAL
    acc = score2;
#endif
  
  double ERRTOL = 1.0e-6;  // ERRTOL undefined in C version repository. 
  amin = dist_min; amax=dist_max;
  assert(amax >= amin - ERRTOL);
  assert(acc >= amin - ERRTOL);
  assert(acc <= amax + ERRTOL);
  score = (amax-acc)/(amax-amin);
  f_score_good = true;
  assert(score >= 0.0 - ERRTOL);
  assert(score <= 1.0 + ERRTOL);

    /*static double best_score = 0;
    if (0 && score > best_score) {
        printf("Old score: %f\n", score); 
        double oscore = qsearch_optimize_tree(dm);
        best_score = score;
        if (oscore > best_score) best_score = oscore;
        
    }*/

   //printf("Optimized score = %f\n", score);
    //printf("nanos org = %d nanos new = %d speedup factor = %f \n", nanos1, nanos2, (double)nanos1/(double)nanos2);
    //printf("Score: %f %f\n", acc, score2);


    if (fabs(score2-acc) > ERRTOL) {

        std::cout << "score difference " << score2 - acc << "\n";
        //std::cout << "score should be " << acc << ", was " << score2 << "\n";

            
        //exit(EXIT_FAILURE);
    }
    //std::cout << "\nQSearchTree::score_tree() complete\n";

  return score;
}

double QSearchTree::score_tree_fast_v2() {
  //std::cout << "\nQSearchTree::score_tree_fast_v2()\n";
  QSearchConnectedNodeMap map(*this);
  
  // run over all pairs
  int node_count = total_node_count;
  int leaf_count = (node_count + 2)/2;
  int i,j;

  double sum = 0.0;
  
  /* loop over internal nodes. As we have know where leaves are, we can now, for each internal node
    * calculate the number of consistent pairs that participate in the sum
    *
    * Suppose we have 8 leafs, and an internal node structured as:
    *
    * branch1 points to leafs 0 3 6 7
    * branch2 points to leafs 2 5
    * branch3 points to leafs 1 4
    *
    * We can, for each branch, calculate the distances that needs to be added to the overall sum 
    *
    * for branch1, we can calculate that there are 6 pairs that are embedded in the tree that form quartets 
    * with all pairs from branch2 and branch3. Thus npairs = 6, and we compute:
    *
    * sum += 6 * ( d(2,1) + d(2,4) + d(5,1) + d(5, 4) )
    *
    * We do not calculate d(2,5), nor d(1,4), as these pairs are taken care of by the internal nodes that
    * split them in different branches
    *
    * By doing the same for branch2 and branch 4, we achieve an overall n^3 algorithm. 
    *
    */
  
  std::vector< std::vector< long long > > tmpmat( node_count, std::vector< long long >(leaf_count * leaf_count, 0ll));

  int node;
  int branch, n, npairs, first, second, ni, nj;
{
#if QSOPENMP_ENABLED
#pragma omp parallel for shared(tmpmat) schedule(dynamic,CHUNKSIZE) private(branch,n,npairs,first,second,i,ni,j,nj)
#endif
    for (node = leaf_count; node < node_count; ++node) {
        for (branch = 0; branch < 3; ++branch) {
            n = map[node].leaf_count[branch];
            
            if (n > 1) { // we need to accumulate the data for all pairs
                npairs = n * (n-1) / 2; // number of pairs
                
                first = (3 + branch - 1) % 3;
                second = (branch + 1) % 3;
                
//              double sumdistance = 0;
                 
                for (i = 0; i < leaf_count; ++i) {
                    
                    ni = leaf_placement[i];
                    if (map[node].node_branch[ni] != first) continue; // this leaf is in the wrong branch
                    
                    for (j = 0; j < leaf_count; ++j) {
                        nj = leaf_placement[j];
                        if ( map[node].node_branch[nj] != second) continue; // this leaf is in the wrong branch
                        
                        if(i!=j) tmpmat[node][i + leaf_count*j] += npairs; 
                        //double dist  = dm[i][j];
                        //sumdistance += dist;
                    }
                }
                
                //sum += npairs * sumdistance;
            }
        }
    }
}
    
    sum = 0.0;
    for (node = leaf_count; node < node_count; ++node) {
    for (i = 0; i < leaf_count; ++i) {
        for (j = 0; j < leaf_count; ++j) {
            sum += tmpmat[node][i + j*leaf_count] * dm[i][j];
        }
    }
    }
  //std::cout << "\nQSearchTree::score_tree_fast_v2() complete\n";

  return sum;
}

std::string QSearchTree::to_dot() {
  std::ostringstream oss;
  oss << "graph \"" << "untitled" << "\" {\n";
  for (int i = 0; i < total_node_count; i += 1) {
    if( ( i < dm.dim ) && dm.has_labels() ) oss << i << " [label=\"" << dm.labels[i] << "\"];\n";
    else oss << i << " [label=\"node " << i << "\"];\n";
  }
  for (int i = 0; i < total_node_count; i += 1) {
    for (int j = i; j < total_node_count; j += 1) {
      if (is_connected(i, j)) {
        oss << i << " -- " << j << " [weight=\"2\"];\n";
      }
    }
  }
  oss << "}\n";
  return oss.str();
}

#include <string>
#include <sstream>

std::string QSearchTree::to_json() {
  std::ostringstream oss;
  oss << "{\n  \"nodes\": [\n";

  for (int i = 0; i < total_node_count; i += 1) {
    oss << "    {\n";
    oss << "      \"index\": " << i << ",\n";
    if ((i < dm.dim) && dm.has_labels()) {
      oss << "      \"label\": \"" << dm.labels[i] << "\",\n";
    } else {
      oss << "      \"label\": \"node " << i << "\",\n";
    }

    // Add connected nodes
    oss << "      \"connections\": [";
    bool first = true;
    for (int j = 0; j < total_node_count; j += 1) {
      if (is_connected(i, j)) {
        if (!first) {
          oss << ", ";
        }
        oss << j;
        first = false;
      }
    }
    oss << "]\n    }";

    if (i < total_node_count - 1) {
      oss << ",\n";
    } else {
      oss << "\n";
    }
  }

  oss << "  ]\n}";
  return oss.str();
}
