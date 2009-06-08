/* Copyright Swann Perarnau 2009
*
*   contributor(s) :  
*
*   contact : firstname.lastname@imag.fr	
*
* This software is a computer program whose purpose is to help the
* random generation of graph structures and adding various properties
* on those structures.
*
* This software is governed by the CeCILL  license under French law and
* abiding by the rules of distribution of free software.  You can  use, 
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info". 
* 
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability. 
* 
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or 
* data to be ensured and,  more generally, to use and operate it in the 
* same conditions as regards security. 
* 
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/
/* GGen is a random graph generator :
 * it provides means to generate a graph following a
 * collection of methods found in the litterature.
 *
 * This is a research project founded by the MOAIS Team,
 * INRIA, Grenoble Universities.
 */


#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/* We use extensively the BOOST library for 
 * handling output, program options and random generators
 */
#include <boost/config.hpp>
#include <boost/program_options.hpp>
#include <boost/graph/graphviz.hpp>

#include "ggen.hpp"
#include "random.hpp"
#include "dynamic_properties.hpp"

using namespace boost;

////////////////////////////////////////////////////////////////////////////////
// Global definitions
////////////////////////////////////////////////////////////////////////////////

// a random number generator for all our random stuff, initialized in main
ggen_rng* global_rng;

////////////////////////////////////////////////////////////////////////////////
// Generation Methods
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////
// Erdos-Renyi : G(n,p)
// One of the simplest way of generating a graph
// Supports :
// 	-- dag option
// 	-- params : number of vertices, probability of an edge
///////////////////////////////////////

/* Run through the adjacency matrix
 * and at each i,j decide if matrix[i][j] is an edge with a given probability
 */
void gg_erdos_gnp(Graph& g, int num_vertices, double p, bool do_dag)
{
	// generate the matrix
	bool matrix[num_vertices][num_vertices];
	int i,j;
	
	for(i=0; i < num_vertices; i++)
	{
		for(j = 0; j < num_vertices ; j++)
		{
			// this test activate always if do_dag is false,
			// only if i < j if do_dag is true
			if(i < j || !do_dag)
			{
				// coin flipping to determine if we add an edge or not
				matrix[i][j] = global_rng->bernoulli(p);
			}
			else
				matrix[i][j] = false;
		}
	}

	// translate the matrix to a graph
	g = Graph(num_vertices);
	std::map < int, Vertex > vmap;
	std::pair < Vertex_iter, Vertex_iter > vp;
	i = 0;
	for(vp = boost::vertices(g); vp.first != vp.second; vp.first++)
		vmap[i++] = *vp.first;

	for(i=0;i < num_vertices; i++)
		for(j=0; j < num_vertices; j++)
			if(matrix[i][j])
				add_edge(vmap[i],vmap[j],g);

}

///////////////////////////////////////
// Random Vertex Pairs
// choose randomly two vertices to add an edge to
// Supports :
// 	-- dag option
// 	-- limited edge number by edge by post edge elminitation
// 	-- adding edges (TODO, how ??)
///////////////////////////////////////


/* Random generation by choosing pairs of vertices.
*/ 
void gg_random_vertex_pairs(Graph& g,int num_vertices, int num_edges, bool allow_parallel = false, bool self_edges = false) {	

	g = Graph(num_vertices);

	// create a two arrays for ggen_rnd::choose
	boost::any *src = new boost::any[num_vertices];
	boost::any *dest = new boost::any[2];
	
	// add all vertices to src
	int i = 0;
	std::pair<Vertex_iter, Vertex_iter> vp;
	for (vp = boost::vertices(g); vp.first != vp.second; ++vp.first)
		src[i++] = boost::any_cast<Vertex>(*vp.first);

	int added_edges = 0;
	while(added_edges < num_edges ) {
		if( !self_edges)
		{
			global_rng->choose(dest,2,src,num_vertices,sizeof(boost::any));
		}
		else
		{
			// TODO rnd.shuffle
		}
		Vertex u,v;
		u = boost::any_cast<Vertex>(dest[0]);
		v = boost::any_cast<Vertex>(dest[1]);
		std::pair<Edge,bool> result = add_edge(u,v,g);
		if(!allow_parallel && result.second)
			added_edges++;
	}

	//delete src;
	//delete dest;
}

////////////////////////////////////////////////////////////////////////////////
// MAIN
////////////////////////////////////////////////////////////////////////////////

namespace po = boost::program_options;
dynamic_properties properties(&create_property_map);
Graph *g;

/* Main program
 */
int main(int argc, char** argv)
{
	// Init the structures
	////////////////////////////

	g = new Graph();
	
	// Handle command line arguments
	////////////////////////////////////
	po::options_description od_general("General Options");
	od_general.add_options()
		("help", "produce help message")

		/* I/O options */
		("output,o", po::value<std::string>(), "Set the output file")
	;
	
	ADD_DBG_OPTIONS(od_general);

	po::options_description od_methods("Methods Options");
	od_methods.add_options()
		("method", po::value<std::string>(),"The generation method to use")
		("method-args",po::value<std::vector<std::string> >(),"The generation method's arguments")
	;


	// Positional Options
	///////////////////////////////

	po::positional_options_description pod_methods;
	pod_methods.add("method", 1);
	pod_methods.add("method-args",-1);

	po::options_description od_all;
	po::options_description od_ro = random_rng_options();

	od_all.add(od_general).add(od_methods).add(od_ro);

	po::variables_map vm_general;
	po::parsed_options prso_general = po::command_line_parser(argc,argv).options(od_all).positional(pod_methods).allow_unregistered().run();
	po::store(prso_general,vm_general);
	po::notify(vm_general);
	
	if (vm_general.count("help")) {
		std::cout << "Usage: " << argv[0] << "[options] method_name method_arguments" << std::endl << std::endl;

		std::cout << od_general << std::endl;
		std::cout << od_ro << std::endl;
		
		std::cout << "Methods Available:" << std::endl;
		std::cout << "erdos_gnp\t\tThe classical adjacency matrix method" << std::endl << std::endl;
		return 1;
	}
	
	if (vm_general.count("output")) 
	{
		// create a new file with 344 file permissions
		int out = open(vm_general["output"].as<std::string>().c_str(),O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	
		// redirect stdout to it
		dup2(out,STDOUT_FILENO);
		close(out);
	}

	// Random number generator, options handling
	global_rng = random_rng_handle_options_atinit(vm_general);



	// Graph methods handling
	////////////////////////////////
	
	// First we recover all the possible options we didn't parse
	// this is the -- options that we didn't recognized
	std::vector< std::string > unparsed_args = po::collect_unrecognized(prso_general.options,po::exclude_positional);
	// this is the positional arguments that were given to the method
	std::vector< std::string > parsed_args = vm_general["method-args"].as < std::vector < std::string > >();
	
	//we merge the whole thing
	std::vector< std::string > to_parse;
	to_parse.insert(to_parse.end(),unparsed_args.begin(),unparsed_args.end());
	to_parse.insert(to_parse.end(),parsed_args.begin(),parsed_args.end());

	if(vm_general.count("method"))
	{
		std::string method_name = vm_general["method"].as<std::string>();
		po::options_description od_method("Method options");
		po::variables_map vm_method;

		if(method_name == "erdos_gnp")
		{
			bool do_dag;
			double prob;
			int nb_vertices;
			// define the options specific to this method
			od_method.add_options()
				("dag",po::bool_switch(&do_dag)->default_value(false),"Generate a DAG instead of a classical graph")
				("nb-vertices,n",po::value<int>(&nb_vertices)->default_value(10),"Set the number of vertices in the generated graph")
				("probability,p",po::value<double>(&prob)->default_value(0.5),"The probability to get each edge");
			
			// define method arguments as positional
			po::positional_options_description pod_method_args;
			pod_method_args.add("nb-vertices",1);
			pod_method_args.add("probability",2);
			
			// do the parsing
			po::store(po::command_line_parser(to_parse).options(od_method).positional(pod_method_args).run(),vm_method);
			po::notify(vm_method);

			gg_erdos_gnp(*g,nb_vertices,prob,do_dag);
		}
		else
		{
			std::cerr << "Error : you must provide a VALID method name!" << std::endl;
			exit(1);
		}
	}
	else
	{
		std::cerr << "Error : you must provide a method name !" << std::endl;
		exit(1);
	}

	
	// since we created the graph from scratch we need to add a property for the vertices
	std::string name("node_id");
	vertex_std_map *m = new vertex_std_map();
	vertex_assoc_map *am = new vertex_assoc_map(*m);
	properties.property(name,*am);
	
	int i = 0;
	std::pair<Vertex_iter,Vertex_iter> vp;
	for(vp = vertices(*g); vp.first != vp.second; vp.first++)
		put(name,properties,*vp.first,boost::lexical_cast<std::string>(i++));

	// Write graph
	////////////////////////////////////	
	write_graphviz(std::cout, *g,properties);

	random_rng_handle_options_atexit(vm_general,global_rng);
	
	delete g;
	return 0;
}
