%module Currex_backend

%include "std_vector.i"
%include "std_string.i"

%template(vector_string) std::vector<std::string>;
%template(vector_ulong) std::vector<unsigned long>;
%template(vector_vector_ulong) std::vector<std::vector<unsigned long>>;

%inline %{
typedef std::vector<std::string> vector_string;
typedef std::vector<unsigned long> vector_ulong;
typedef std::vector<std::vector<unsigned long>> vector_vector_ulong;
%}


/* d */
%inline %{
void D_set_from_string(std::string const&);
void D_set_file(std::string const&);
typedef unsigned int D_flag_type;
const D_flag_type D_flag_lib = 1<<0;
const D_flag_type D_flag_lib_throw = 1<<1;
const D_flag_type D_flag_ofp_throw = 1<<2;
void D_set_xparam(D_flag_type);
%}


/* pruner */
%inline %{
vector_string prune_vertices(vector_string const&);
%}

/* rate */
%inline %{
struct Rate {
        std::string instrument;
        double bid;
        double ask;
};
%}

%template(vector_rate) std::vector<Rate>;
%inline %{
typedef std::vector<Rate> vector_rate;
%}

/* labeled  */
%{
struct Labeled_graph;
%}
%inline %{
Labeled_graph* new_Labeled_graph();
void delete_Labeled_graph(Labeled_graph*);
void Labeled_graph_filedump(Labeled_graph const&, std::string const&);
vector_string Labeled_graph_labels(Labeled_graph const&);
%}

/* g-rategraph */
%inline %{
namespace g_rategraph {

struct Rated_path {
        vector_ulong path;
        double lrate;
        vector_ulong get_path() const;
};
}
%}

/* graph */
%inline %{
namespace graph {
struct Reload {
        vector_ulong removed_vertices;
        vector_vector_ulong removed_edges; 
        vector_ulong new_vertices;
        vector_vector_ulong new_edges;
};

Reload load_graph_from_rates(Labeled_graph&, vector_rate const&);

g_rategraph::Rated_path best_path(Labeled_graph const&, long max_iterations = -1);

}
%}



