// vim:sw=4:ts=4:cindent
/*
   This file is part of mist2.

   mist2 is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   mist2 is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mist2; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   Copyright 2002-2007 Pierre Ganty, Laurent Van Begin
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <getopt.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <assert.h>

#include "laparser.h"
#include "ist.h"


/* For printing the error trace */
void ist_print_error_trace(ISTSharingTree * initial_marking,THeadListIST * list_ist, transition_system_t * rules) 
{
	ISTSharingTree *S, *post, *Siter, *intersect;
	size_t i;
	boolean Continue;

	printf("Run to reach the bad states from the initial marking:\n");

	S=ist_copy(initial_marking);
	Siter = ist_first_element_list_ist(list_ist);
	while (Siter != NULL) {
		i = 0;
		Continue = true;
		while ((i < rules->limits.nbr_rules) && (Continue == true)) {
			post = ist_enumerative_post_transition(S,rules,i);
			intersect = ist_intersection(post,Siter);
			ist_dispose(post);
			if (!ist_is_empty(intersect)) { 
				printf("[%d> ",i);
				ist_dispose(S);
				S = intersect;
				Continue = false;
			} else { 
				ist_dispose(intersect);
				++i;
			} 
		} 
		if (i == rules->limits.nbr_rules) { 
			err_msg("\nError: No Path Found!\n");
			Siter = NULL;
		} else 
			Siter = ist_next_element_list_ist(list_ist);
	}
	printf("\n");
}

boolean backward_reachability(system, initial_marking, frontier) 
	transition_system_t *system;
	ISTSharingTree *frontier, *initial_marking;
{
	ISTSharingTree *old_frontier, *temp, *reached_elem;
	size_t nbr_iteration;
	boolean Continue;
	boolean reached;
	THeadListIST List;
	
	/* Now we declare variables to measure the time consumed by the function */
	long int tick_sec=0 ;
	struct tms before, after;
	float comp_u, comp_s ;

	times(&before) ;

	temp = ist_remove_with_all_invar_exact(frontier, system);
	/* We de not call ist_dispose(frontier); since we do not want
	 * to modify the IN parameter */
	frontier = temp;
	/* reached_elem is set to frontier that is in the "worst case" the empty IST */
	reached_elem = ist_copy(frontier);

	Continue = true;
	temp=ist_intersection(initial_marking,reached_elem);
	reached = (ist_is_empty(temp) == true ? false : true);
	ist_dispose(temp);
	if (ist_is_empty(frontier) == true ||  reached == true) {
		Continue = false;
		puts("Unsafe region is empty or lies outside the invariants or contrains some initial states");
	}
	ist_init_list_ist(&List);
	nbr_iteration = 1;
	while (Continue == true) {
		printf("\n\nIteration\t%3d\n", nbr_iteration);
		puts("Computation of the symbolic predecessors states ...");
		old_frontier = frontier;
		/* As post cond, we have that frontier is minimal (in a 1to1 comparison) w.r.t reached_elem */
		frontier = ist_pre_pruned_wth_inv_and_prev_iterates(old_frontier, reached_elem, system);

		if (ist_is_empty(frontier) == false) {

			printf("The new frontier counts :\n");
			ist_checkup(frontier);
#ifdef VERBOSE
			ist_write(frontier);
#endif
			temp=ist_intersection(initial_marking,frontier);
			reached = (ist_is_empty(temp) == true ? false : true);
			ist_dispose(temp);
			if (reached == true) {
				puts("+-------------------------------------+");
				puts("|Initial states intersect the frontier|");
				puts("+-------------------------------------+");
				Continue = false;
				ist_insert_at_the_beginning_list_ist(&List,old_frontier);
				/* 
				 * Exact subsumption test is relevant ONLY FOR INTERVALS 
				 *			 else if (ist_exact_subsumption_test(frontier,reached_elem))
				 *				printf("reached_elem subsumes frontier \n");
				 *				Continue = false; 
				 */
			} else {
				temp = ist_remove_subsumed_paths(reached_elem, frontier);
				/* 
				 * Here we prune trivial 1to1 inclusion 
				 * We don't use simulation relations, we compute it exactly !
				 */
				ist_dispose(reached_elem);
				reached_elem = ist_union(temp, frontier);
				/* To minimize we can use:
				 * ist_minimal_form or ist_minimal_form_sim_based
				 */
				puts("After union, the reached symbolic state space is:");
				ist_checkup(reached_elem);
				ist_insert_at_the_beginning_list_ist(&List,old_frontier);
				old_frontier = frontier;
			}

		} else 
			Continue = false;
		nbr_iteration++;
	}

	if (nbr_iteration != 0){
		puts("The reached symbolic state space is:");
		ist_stat(reached_elem);
#ifdef VERBOSE
		ist_write(reached_elem);
#endif
	}
	if (reached == true)
		ist_print_error_trace(initial_marking,&List,system);
	ist_empty_list_ist_with_info(&List);
	times(&after);
	tick_sec = sysconf (_SC_CLK_TCK) ;
	comp_u = ((float)after.tms_utime - (float)before.tms_utime)/(float)tick_sec ;
	comp_s = ((float)after.tms_stime - (float)before.tms_stime)/(float)tick_sec ;
	printf("Total time of computation (user)   -> %6.3f sec.\n",comp_u);
	printf("                          (system) -> %6.3f sec.\n",comp_s);

	ist_dispose(reached_elem);
	
	/* Is the system safe ? */
	return (reached== true ? false : true);
}

static void print_version() {
	printf("Version %s\n", VERSION);
}

static void print_help() 
{
	puts("Usage: mist2 [options] filename\n");
	puts("Options:");
	puts("     --help, -h   this help");
	puts("     --version    show version numbers");
}

static void head_msg()
{
	puts("Copyright (C) 2002-2007 Pierre Ganty, Laurent Van Begin.");
	puts("mist2 is free software, covered by the GNU General Public License, and you are");
	puts("welcome to change it and/or distribute copies of it under certains conditions.");
	puts("There is absolutely no warranty for mist2. See the COPYING for details.");
}

static void mist_cmdline_options_handle(int argc, char *argv[ ]) 
{
	int c;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			{"version", 0, 0, 0},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, "h", long_options, &option_index);
		if (c == -1)
			break;

		switch (c)
		{
			case 0:
				if (strcmp(long_options[option_index].name,"version") == 0) {
					print_version();
					exit(EXIT_SUCCESS);
				} 
				break;

			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;

			default:
				print_help();
				err_quit("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind >= argc) {
		print_help();
		err_quit("Missing filename");
	}
}

/*
 * apply the (0,...,k,INFINITY) abstraction.
 * POST: for each layer the list of nodes remains sorted.
 */
void abstract_bound(ISTSharingTree *S, integer16 *bound) 
{
	ISTLayer *L;
	ISTNode *N;
	size_t i;

	for(L = S->FirstLayer, i = 0 ; L != S->LastLayer ; L = L->Next, i++) 
		for(N = L->FirstNode ; N != NULL ; N = N->Next) 
			if (!ist_less_or_equal_value(N->Info->Right,bound[i]))
				N->Info->Right=INFINITY;
}

void bound_values(ISTSharingTree *S, integer16 *bound)
{
	ISTLayer *L;
	ISTNode *N;
	size_t i;
	
	for(i=0,L = S->FirstLayer;L != S->LastLayer;i++,L = L->Next)
		for(N= L->FirstNode;N != NULL;N = N->Next)
			ist_assign_values_to_interval(N->Info,min(N->Info->Left,bound[i]),min(N->Info->Right,bound[i]));
}


/* remove nodes with unbounded value */
void RemoveUnboundedNodes(ISTSharingTree *S) 
{
	ISTLayer *Layer;
	ISTNode *Node;
	boolean stop;
	Layer=S->FirstLayer;
	while(Layer!=S->LastLayer) {
		Node = Layer->FirstNode;
		while (Node != NULL){
			if (ist_greater_or_equal_value(Node->Info->Right,INFINITY)==true)
				ist_remove_sons(Node);
			Node = Node->Next;
		}
		Layer=Layer->Next;
	}
	ist_remove_node_without_father(S);
	ist_remove_node_without_son(S);
	/* Remove empty layers if any */
	stop = false;
	while (!stop) {
		if (S->LastLayer == NULL)
			stop = true;
		else {
			if (S->LastLayer->FirstNode != NULL)
				stop = true;
			else
				ist_remove_last_layer(S);
		}
	}
	if (!ist_is_empty(S)) 
		ist_adjust_second_condition(S);
	//assert(ist_checkup(S)==true);
}


/*
 * lfp is a out parameter which is 
 * 1) an inductive invariant of the system
 * 2) a dc-set
 * WORKS FOR PETRI NET ONLY
 */
boolean eec_fp(system, abs, initial_marking, bad, lfp)
	transition_system_t *system;
	abstraction_t *abs; /* For the bounds on the places */
	ISTSharingTree *initial_marking, *bad, **lfp;
{
	boolean retval;
	ISTSharingTree *abs_post_star, *inter, *downward_closed_initial_marking, *bpost, *tmp, *_tmp;
	boolean finished;
	size_t i;
	
	float comp_u,comp_s; 
	long int tick_sec=0 ;
	struct tms before, after;
	
	times(&before);	

	
	downward_closed_initial_marking = ist_downward_closure(initial_marking);
	ist_normalize(downward_closed_initial_marking);
	assert(ist_checkup(downward_closed_initial_marking)==true);

	finished=false;
	while (finished == false) {
		printf("eec: ENLARGE begin\t");
		fflush(NULL);
		/* To OVERapproximate we use abstract_bound */
		abs_post_star = ist_abstract_post_star(downward_closed_initial_marking,abstract_bound,abs->bound,system);
		assert(ist_checkup(abs_post_star)==true);
		puts("end");
		*lfp = abs_post_star;
		inter = ist_intersection(abs_post_star,bad);
		finished=ist_is_empty(inter);
		ist_dispose(inter);
		if (finished==true) 
			/* finished==true -> the system is safe */
			retval = true;
		else {
			printf("eec: EXPAND begin\t");
			fflush(NULL);
			/* use bpost = ist_abstract_post_star(downward_closed_initial_marking,bound_values,abs->bound,system)
			 * if you want to compute the lfp. Instead we make something more
			 * efficient by testing, at each iteration, for the emptiness of
			 * intersection w/ bad. */

			bpost = ist_copy(downward_closed_initial_marking);
			bound_values(bpost,abs->bound);
			ist_normalize(bpost);
			inter=ist_intersection(bpost,bad);
			finished= ist_is_empty(inter) == true ? false : true;
			ist_dispose(inter);
			while (finished==false) {
				tmp = ist_abstract_post(bpost,bound_values,abs->bound,system);
				_tmp =  ist_remove_subsumed_paths(tmp,bpost);
				ist_dispose(tmp);
				if (ist_is_empty(_tmp)==false) {		
					inter=ist_intersection(_tmp,bad);
					finished=ist_is_empty(inter) == true ? false : true;
					ist_dispose(inter);
					tmp = ist_remove_subsumed_paths(bpost,_tmp);
					ist_dispose(bpost);
					bpost = ist_union(tmp,_tmp);
					ist_dispose(tmp);
					ist_dispose(_tmp);
				} else {
					ist_dispose(_tmp);
					break;
				}
			}
			assert(ist_checkup(bpost)==true);
			ist_dispose(bpost);
			puts("end");
			if (finished==true)
				/* finished==true -> we hitted the bad states, the system is unsafe */
				retval=false;
			else {
				/* One more iteration is needed because both bpost and
				 * abs_post_star does not allow to conclude */
				ist_dispose(abs_post_star);
				printf("eec: BOUNDS\t");
				for(i=0;i<abs->nbV;printf("%d ",++abs->bound[i++]));
				printf("\n");
			}
		}
	}
	ist_dispose(downward_closed_initial_marking);

	times(&after);
	tick_sec = sysconf (_SC_CLK_TCK) ;
	comp_u = ((float)after.tms_utime - (float)before.tms_utime)/(float)tick_sec ;
	comp_s = ((float)after.tms_stime - (float)before.tms_stime)/(float)tick_sec ;
	printf("Total time of computation (user)   -> %6.3f sec.\n",comp_u);
	printf("                          (system) -> %6.3f sec.\n",comp_s);	
	
	return retval;
}

/*
 * List is a out parameter from which we 
 * will extract the counterexample
 * WORKS FOR PETRI NET ONLY
 */
boolean eec_cegar(system, abs, initial_marking, bad, List)
	transition_system_t *system;
	abstraction_t *abs; /* For the bounds on the places */
	ISTSharingTree *initial_marking, *bad;
	THeadListIST *List;
{
	boolean retval;
	ISTSharingTree *abs_post_star, *inter, *downward_closed_initial_marking, *bpost, *tmp, *_tmp;
	boolean finished;
	size_t i;

	float comp_u,comp_s; 
	long int tick_sec=0 ;
	struct tms before, after;

	times(&before);	

	downward_closed_initial_marking=ist_downward_closure(initial_marking);
	ist_normalize(downward_closed_initial_marking);
	assert(ist_checkup(downward_closed_initial_marking)==true);

	finished=false;
	while (finished == false) {
		printf("eec: ENLARGE begin\t");
		fflush(NULL);
		/* To OVERapproximate we use abstract_bound */
		abs_post_star = ist_abstract_post_star(downward_closed_initial_marking,abstract_bound,abs->bound,system);
		assert(ist_checkup(abs_post_star)==true);
		puts("end");
		inter = ist_intersection(abs_post_star,bad);
		ist_dispose(abs_post_star);
		finished=ist_is_empty(inter);
		ist_dispose(inter);
		if (finished==true) 
			/* finished==true -> the system is safe */
			retval = true;
		else {
			/* Pay attention here, initial_marking might contain infinitely
			 * many markings; so in the expand phase we work with a finite
			 * subset of it. */
			printf("eec: EXPAND begin\t");
			fflush(NULL);
			ist_init_list_ist(List);

			/* expand works w/ dc-sets */
			bpost = ist_copy(downward_closed_initial_marking);
			bound_values(bpost,abs->bound);
			ist_normalize(bpost);
			inter=ist_intersection(bpost,bad);
			finished= ist_is_empty(inter) == true ? false : true;
			ist_dispose(inter);
			/* insert a copy of initial_marking in the list */
			ist_insert_at_the_beginning_list_ist(List,ist_copy(initial_marking));
			while (finished==false) {
				tmp = ist_abstract_post(bpost,bound_values,abs->bound,system);
				_tmp =  ist_remove_subsumed_paths(tmp,bpost);
				ist_dispose(tmp);
				if (ist_is_empty(_tmp)==false) {		
					/* insert a copy of _tmp in the list */
					ist_insert_at_the_beginning_list_ist(List,ist_copy(_tmp));
					inter=ist_intersection(_tmp,bad);
					finished=!ist_is_empty(inter);
					ist_dispose(inter);
					tmp=ist_remove_subsumed_paths(bpost,_tmp);
					ist_dispose(bpost);
					bpost = ist_union(tmp,_tmp);
					ist_dispose(tmp);
					ist_dispose(_tmp);
				} else {
					// we reached a fixpoint, so exit of the loop
					ist_dispose(_tmp);
					break;
				}
			}
			assert(ist_checkup(bpost)==true);
			ist_dispose(bpost);
			puts("end");
			if (finished==true)
				/* finished==true -> we hitted the bad states, the system is unsafe */
				retval=false;
			else {
				/* One more iteration is needed because both bpost and
				 * abs_post_star does not allow to conclude. 
				 * Free the list of sharing tree (here info means st) */
				ist_empty_list_ist_with_info(List);
				printf("eec: BOUNDS\t");
				for(i=0;i<abs->nbV;printf("%d ",++abs->bound[i++]));
				printf("\n");
			}
		}
	}
	ist_dispose(downward_closed_initial_marking);

	times(&after);
	tick_sec = sysconf (_SC_CLK_TCK) ;
	comp_u = ((float)after.tms_utime - (float)before.tms_utime)/(float)tick_sec ;
	comp_s = ((float)after.tms_stime - (float)before.tms_stime)/(float)tick_sec ;
	printf("Total time of computation (user)   -> %6.3f sec.\n",comp_u);
	printf("                          (system) -> %6.3f sec.\n",comp_s);	

	return retval;
}

void cegar(system, initial_marking, bad) 
	transition_system_t *system;
	ISTSharingTree *bad, *initial_marking;
{
	abstraction_t *myabs, *newabs, *abs_tmp;
	transition_system_t *sysabs;
	ISTSharingTree *tmp, *_tmp, *alpha_bad, *predecessor, *inter, *seed, *cutter, *Z;
	ISTLayer *layer;
	ISTNode *node;
	THeadListIST cex;
	size_t i, nb_iteration, lg_cex;
	int *countex, j;
	boolean out, conclusive, eec_conclusive;

	printf("CEGAR..\n");
	tmp=ist_intersection(initial_marking,bad);
	conclusive = (ist_is_empty(tmp)==true ? false : true);
	ist_dispose(tmp);

	// since new_abstraction works w/ dc-sets we compute \neg bad 
	tmp=ist_copy(bad);
	ist_complement(tmp,system->limits.nbr_variables);
	// _tmp is a dc-set but we want each path to be dc-closed
	_tmp=ist_downward_closure(tmp);
	ist_normalize(_tmp);
	ist_dispose(tmp);

	/* the initial abstraction is given by refinement(_tmp) */
	myabs=new_abstraction_dc_set(_tmp,system->limits.nbr_variables);

	nb_iteration=0;
	while(conclusive == false) {
		puts("begin of iteration");
		// We build the abstract system 
		sysabs=build_sys_using_abs(system,myabs);
		puts("The current abstraction is :");
		print_abstraction(myabs);
		//puts("The current abstracted net is:");
		//print_transition_system(sysabs);
		//Set tmp=alpha(initial_marking), alpha_bad 
		tmp = ist_abstraction(initial_marking,myabs);
		alpha_bad = ist_abstraction(bad,myabs);

		eec_conclusive=eec_cegar(sysabs, myabs, tmp, alpha_bad, &cex);
		ist_dispose(tmp);

		if (eec_conclusive==true) {
			// says "safe" because it is indeed safe 
			puts("EEC concludes safe with the abstraction");
			print_abstraction(myabs);
			conclusive = true;
		} else {
			/* compute the length of cex */
			lg_cex=ist_count_elem_list_ist(&cex)-1;
			printf("run to bad is %d transition(s) long.\n",lg_cex);
			tmp=ist_first_element_list_ist(&cex);

			/* seed is neither a dc-set, nor a uc-set */
			seed=ist_intersection(alpha_bad,tmp);
			/* so we compute the uc_closure. This should not break the
			 * invariant that nodes, sons, etc are ordered. */
			layer=seed->FirstLayer;	
			while(layer!=seed->LastLayer) {
				node=layer->FirstNode;
				while(node!=NULL){
					node->Info->Right=INFINITY;
					node=node->Next;
				}
				layer=layer->Next;
			}
			ist_normalize(seed);

			/* extract the cex */
			countex=(int *)xmalloc(lg_cex*sizeof(int));

			puts("seed of the abstract cex (backward analysis)");
			ist_write(seed);
			_tmp=seed;

			for(j=0;j<lg_cex;j++) {
				i=0;
				out=false;
				/* Take the next elem in the list */
				cutter=ist_next_element_list_ist(&cex);
				while((i < sysabs->limits.nbr_rules) && (out == false)) {
					/* computes the minpre[transition[i]] */
					predecessor=ist_pre_of_rule_plus_transfer(_tmp,&sysabs->transition[i]);
					/* as specified in ist_pre_of_rule_plus_transfer the result is not
					 * supposed to be in normal form. */
					if (!ist_is_empty(predecessor))
						ist_normalize(predecessor);
					/* and intersect with the dc-set computed during expand */
					inter=ist_intersection(cutter,predecessor);	
					assert(ist_checkup(inter)==true);
					ist_dispose(predecessor);
					if (!ist_is_empty(inter)) {
						/* Compute the uc-closure of inter. This should not
						 * break the invariant that nodes, sons, etc are
						 * ordered. */
						layer=inter->FirstLayer;	
						while(layer!=inter->LastLayer) {
							node=layer->FirstNode;
							while(node!=NULL){
								node->Info->Right=INFINITY;
								node=node->Next;
							}
							layer=layer->Next;
						}
						ist_normalize(inter);
						countex[j]=i;
						ist_write(inter);
						ist_dispose(_tmp);
						_tmp=inter;
						out=true;
					} else {
						ist_dispose(inter);
						++i;
					}

				}
			}
			/* release the list of dc-sets from which the cex is extracted. this list is now useless */
			ist_empty_list_ist_with_info(&cex);
			puts("The counter example for the abstract net");		
			for(j=0;j<lg_cex;printf("<%d]",countex[j++]));
			printf("\n");

			/* We have countex, we need a seed which is obtained by replaying
			 * forward the countex starting from inter.  Inter is a uc-set
			 * whose minimal elements are given by the intersection of
			 * initial_marking with the uc-set computed previously. Inter
			 * remains uc-closed after firing the transitions of countex. */
			tmp=inter;
			j=lg_cex-1;
			while(j>=0) {
				_tmp=ist_enumerative_post_transition(tmp,sysabs,countex[j--]);
				ist_dispose(tmp);
				tmp=_tmp;
			}
			/* we take the minimal elements of the outcoming uc-set */
			layer=tmp->FirstLayer;	
			while(layer!=tmp->LastLayer) {
				node=layer->FirstNode;
				while(node!=NULL){
					node->Info->Right=node->Info->Left;
					node=node->Next;
				}
				layer=layer->Next;
			}
			ist_normalize(tmp);

			/* seed=gamma(tmp); then take intersection with bad */
			seed=ist_concretisation(tmp,myabs);
			predecessor=ist_intersection(seed,bad);
			puts("seed of the concrete cex (backward analysis)");
			assert(ist_checkup(predecessor)==true);

			/* analysis of the counter example in the concrete */
			j=0;
			do {
				ist_dispose(tmp);
				tmp=predecessor;
				predecessor=ist_symbolic_pre_of_rule(tmp, &system->transition[countex[j++]]);
			} while(!ist_is_empty(predecessor) && j<lg_cex);
			Z=NULL;
			/* if we went through the whole counterexample then we found a real counterexample */
			if(!ist_is_empty(predecessor)) {
				_tmp=ist_intersection(initial_marking,predecessor);
				conclusive=!ist_is_empty(_tmp);
				ist_dispose(_tmp);
				if (conclusive==true) {
					puts("Find a real counterexample");
					for(j=0;j<lg_cex;printf("<%d]",countex[j++]));
					printf("\n");
				} else
					Z=ist_copy(predecessor);
			} else 
				Z=ist_copy(tmp);
			/* release predecessor, tmp */
			ist_dispose(predecessor);
			ist_dispose(tmp);
			// Z is the last non empty pre_computation
			// We refine by representing exactly Z; Z are bad
			// states in the sense of CEGAR (i.e. unreachable but lead to bad)
			if(Z!=NULL) {
				puts("Z is");
				assert(ist_checkup(Z)==true);
				/* new_abstraction_finite_set is tailored for Z which is finite */
				puts("building new absraction");
				abs_tmp = new_abstraction_finite_set(Z,system->limits.nbr_variables);
				puts("abs_tmp");
				print_abstraction(abs_tmp);
				newabs = glb(abs_tmp,myabs);
				// release abs_tmp 
				dispose_abstraction(abs_tmp);
				// release myabs
				dispose_abstraction(myabs);
				myabs = newabs;
			}
		}
	}
	// release sysabs 
	dispose_transition_system(sysabs);
	printf("end of iteration %d\n",++nb_iteration);
}

void ic4pn(system, initial_marking, bad) 
	transition_system_t *system;
	ISTSharingTree *bad, *initial_marking;
{
	abstraction_t *myabs, *newabs, *abs_tmp;
	transition_system_t *sysabs;
	ISTSharingTree *lfp_eec, *gamma_gfp, *tmp, *_tmp, *Z, *aZ, *neg_aZ,
				   *iterates, *new_iterates, *alpha_initial_marking, *rfp;
	size_t i,j,nb_iteration, iterator;
	boolean out, conclusive, eec_conclusive;

	//////////////////////////////////////////////////////////////////
	// creation of an abstraction that corresponds to the system    //
	// => to obtain stat.                                           //
	// ///////////////////////////////////////////////////////////////
	abstraction_t *systemabs;
	systemabs=(abstraction_t *)xmalloc(sizeof(abstraction_t));
	systemabs->nbConcreteV=system->limits.nbr_variables;
	systemabs->nbV=system->limits.nbr_variables;
	systemabs->bound=(integer16 *)xmalloc(systemabs->nbV*sizeof(integer16));
	systemabs->A=(integer16 **)xmalloc(systemabs->nbV*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i)
		systemabs->A[i]=(integer16 *)xmalloc(system->limits.nbr_variables*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i) {
		systemabs->bound[i]=1;
		for(j=0;j<system->limits.nbr_variables;++j)
			systemabs->A[i][j]=1;
	}
	/* printf("EEC for the concrete system\n");
	eec_conclusive=eec_fp(system,systemabs,initial_marking,bad,&lfp_eec);
	if (eec_conclusive == true)
		printf("Answer = true\n");
	else
		printf("Answer = false\n"); */
	printf("IC4PN..\n");

	////////////////////////////////////////////////////////////////////
	//end of eec for the concrete system                              //
	////////////////////////////////////////////////////////////////////
	
	tmp=ist_intersection(initial_marking,bad);
	conclusive = (ist_is_empty(tmp)==true ? false : true);
	ist_dispose(tmp);

	// Z = \neg bad 
	tmp=ist_copy(bad);
	ist_complement(tmp,system->limits.nbr_variables);
	// tmp represents a dc-set 
	Z=ist_downward_closure(tmp);
	ist_normalize(Z);
	ist_dispose(tmp);
	// Z = tmp and each path is a dc-closed 
	
	/* A_0 = refinement(Z_0) */
	myabs=new_abstraction_dc_set(Z,system->limits.nbr_variables);

	nb_iteration=0;
	while(conclusive == false) {
		puts("begin of iteration");
		// We build the abstract system 
		sysabs=build_sys_using_abs(system,myabs);
		puts("The current abstraction is :");
		print_abstraction(myabs);
//		puts("The current abstracted net is:");
//		print_transition_system(sysabs);
		// Set aZ, neg_aZ, alpha_initial_marking 
		aZ = ist_abstraction(Z,myabs);
		assert(ist_checkup(aZ)==true);
		neg_aZ = ist_copy(aZ);
		ist_complement(neg_aZ, sysabs->limits.nbr_variables);
		assert(ist_checkup(neg_aZ)==true);
		alpha_initial_marking = ist_abstraction(initial_marking,myabs);
		assert(ist_checkup(alpha_initial_marking)==true);

		eec_conclusive=eec_fp(sysabs,myabs,alpha_initial_marking, neg_aZ ,&lfp_eec);

		// release neg_aZ
		ist_dispose(neg_aZ);

		if (eec_conclusive==true) {
			// says "safe" because it is indeed safe 
			puts("EEC concludes safe with the abstraction");
			print_abstraction(myabs);
			conclusive = true;
		} else { 
			puts("The EEC fixpoint");
			assert(ist_checkup(lfp_eec)==true);
			ist_write(lfp_eec);
			puts("----------------");

			// rfp is given by lfp_eec /\ aZ
			rfp = ist_intersection(aZ, lfp_eec);
			// release lfp_eec
			ist_dispose(lfp_eec);
			// release aZ
			ist_dispose(aZ);
			tmp = ist_downward_closure(rfp);
			ist_normalize(tmp);
			ist_dispose(rfp);
			rfp=ist_minimal_form(tmp);

			// iterates = rfp 
			iterates = ist_copy(rfp);

			// compute the gfp for the abstraction 
			iterator=0;
			do {
				++iterator;

				assert(ist_checkup(iterates)==true);
				assert(ist_nb_layers(iterates)-1==myabs->nbV);
				//tmp = adhoc_pretild(iterates,sysabs);

				tmp = ist_symbolic_abstract_pre_tild(iterates,sysabs);

				new_iterates = ist_intersection(tmp,rfp);
				assert(ist_checkup(new_iterates)==true);
				ist_dispose(tmp);
				tmp = ist_downward_closure(new_iterates);
				ist_normalize(tmp);
				ist_dispose(new_iterates);
				new_iterates = ist_minimal_form(tmp);
				ist_dispose(tmp);
				assert(ist_checkup(new_iterates)==true);

				// We remove the subsumed paths in iterates 
				tmp = ist_remove_subsumed_paths(iterates,new_iterates);
				out = ist_is_empty(tmp);
				ist_dispose(tmp);
				ist_dispose(iterates);
				iterates = new_iterates;
			} while(out == false);
			printf("number of iterations for the gfp = %d\n",iterator);

			tmp = ist_remove_subsumed_paths(alpha_initial_marking, iterates);
			conclusive=(ist_is_empty(tmp)==true ? false : true);
			ist_dispose(tmp);
			// release alpha_initial_marking
			ist_dispose(alpha_initial_marking);
			// release rfp
			ist_dispose(rfp);
			// We refine the abstraction 
			if(conclusive == false) {
				// we compute gamma(gfp) 
				gamma_gfp = ist_concretisation(iterates,myabs);
				// release iterates
				ist_dispose(iterates);

				//debug pour voire...
/*				printf("Suspens...est-ce que c'est rapide?\n");
				ISTSharingTree * b = ist_symbolic_pre_tild(gamma_gfp,system);
				ist_checkup(b);
*/

				// We compute a concrete iterates 
/*				puts("gamma_gfp");
				ist_write(gamma_gfp);
				assert(ist_checkup(gamma_gfp)==true);
				tmp=ist_copy(gamma_gfp);
				ist_complement(tmp,system->limits.nbr_variables);
				assert(ist_checkup(tmp)==true);
				puts("¬ (gamma_gfp)");
				ist_write(tmp);
				_tmp=ist_pre(tmp,system);
				assert(ist_checkup(_tmp)==true);
				ist_dispose(tmp);
				puts("pre o ¬ (gamma_gfp)");
				ist_complement(_tmp,system->limits.nbr_variables);
				tmp=ist_downward_closure(_tmp);
				ist_normalize(tmp);
				ist_dispose(_tmp);
				_tmp=ist_minimal_form(tmp);
				ist_dispose(tmp);
				assert(ist_checkup(_tmp)==true);
*/
				//debug
/*				ISTSharingTree * a = ist_symbolic_pre_tild(gamma_gfp,system);
				ist_checkup(a);

				a = ist_downward_closure(a);
				ist_normalize(a); 
				a = ist_minimal_form(a);
				printf("assert...\n");
				assert(ist_exact_subsumption_test(a,_tmp) 
				&& ist_exact_subsumption_test(_tmp,a));
				ist_checkup(a);
				ist_checkup(_tmp);
*/
				_tmp = ist_symbolic_pre_tild(gamma_gfp,system);

				// Now intersects w/ gamma_gfp
				tmp=ist_intersection(gamma_gfp,_tmp);
				//release gamma_gfp
				ist_dispose(gamma_gfp);
				ist_dispose(_tmp);
				_tmp=ist_downward_closure(tmp);
				ist_dispose(tmp);
				ist_normalize(_tmp);
				//release Z
				ist_dispose(Z);
				Z=ist_minimal_form(_tmp);
				ist_dispose(_tmp);

				puts("new Z");
				ist_write(Z);
				abs_tmp=new_abstraction_dc_set(Z,system->limits.nbr_variables);
				puts("abs_tmp");
				print_abstraction(abs_tmp);
				newabs = glb(abs_tmp,myabs);
				// release abs_tmp 
				dispose_abstraction(abs_tmp);
				// release myabs
				dispose_abstraction(myabs);
				myabs = newabs;
			} 			
		}
		// release sysabs 
		dispose_transition_system(sysabs);
		printf("end of iteration %d\n",++nb_iteration);
	}
}


/* Assume initial_marking is a downward closed marking and the ist is minimal */
boolean ist_abstract_post_star_bis(ISTSharingTree *initial_marking, void
		(*approx)(ISTSharingTree *S, integer16* b), integer16 *bound,
		transition_system_t *t, ISTSharingTree * bad) 
{
	ISTSharingTree *S, *tmp, *_tmp, *inter;
	boolean result = false;

	S = ist_copy(initial_marking);
	if(approx)
		approx(S,bound);
	ist_normalize(S);
	while (true) {
		tmp = ist_abstract_post(S,approx,bound,t);
		_tmp =  ist_remove_subsumed_paths(tmp,S);
		ist_dispose(tmp);
		if (ist_is_empty(_tmp)==false) {	
			inter = ist_intersection(_tmp,bad);
			if (ist_is_empty(inter) == false) {
				result = true;
				ist_dispose(inter);
				break;
			}	
			ist_dispose(inter);
			tmp = ist_remove_subsumed_paths(S,_tmp);
			ist_dispose(S);
			S = ist_union(tmp,_tmp);
			ist_dispose(tmp);
			ist_dispose(_tmp);
		} else {
			ist_dispose(_tmp);
			break;
		}
	}
	ist_dispose(S);
	return result;	
}

boolean eec_fp_bis(system, abs, initial_marking, bad)
	transition_system_t *system;
	abstraction_t *abs; /* For the bounds on the places */
	ISTSharingTree *initial_marking, *bad;
{
	boolean retval = false;
	ISTSharingTree *inter, *downward_closed_initial_marking, *bpost, *tmp, *_tmp;
	boolean finished;
	size_t i;
	
	float comp_u,comp_s; 
	long int tick_sec=0 ;
	struct tms before, after;
	
	times(&before);	

	
	downward_closed_initial_marking = ist_downward_closure(initial_marking);
	ist_normalize(downward_closed_initial_marking);
	assert(ist_checkup(downward_closed_initial_marking)==true);

	finished=false;
	while (finished == false) {
		printf("eec: ENLARGE begin\t");
		fflush(NULL);
		/* To OVERapproximate we use abstract_bound */
		finished = ! ist_abstract_post_star_bis(downward_closed_initial_marking,abstract_bound,abs->bound,system,bad);
		puts("end");
		if (finished==true) 
			/* finished==true -> the system is safe */
			retval = true;
		else {
			printf("eec: EXPAND begin\t");
			fflush(NULL);
			/* use bpost = ist_abstract_post_star(downward_closed_initial_marking,bound_values,abs->bound,system)
			 * if you want to compute the lfp. Instead we make something more
			 * efficient by testing, at each iteration, for the emptiness of
			 * intersection w/ bad. */

			bpost = ist_copy(downward_closed_initial_marking);
			bound_values(bpost,abs->bound);
			ist_normalize(bpost);
			inter=ist_intersection(bpost,bad);
			finished= ist_is_empty(inter) == true ? false : true;
			ist_dispose(inter);
			while (finished==false) {
				tmp = ist_abstract_post(bpost,bound_values,abs->bound,system);

				printf("tmp\n");
				ist_checkup(tmp);

				_tmp =  ist_remove_subsumed_paths(tmp,bpost);

				printf("_tmp\n");
				ist_checkup(_tmp);

				ist_dispose(tmp);
				if (ist_is_empty(_tmp)==false) {		
					inter=ist_intersection(_tmp,bad);
					finished=ist_is_empty(inter) == true ? false : true;
					ist_dispose(inter);
					tmp = ist_remove_subsumed_paths(bpost,_tmp);
					ist_dispose(bpost);
					bpost = ist_union(tmp,_tmp);
					ist_dispose(tmp);
					ist_dispose(_tmp);
				} else {
					ist_dispose(_tmp);
					break;
				}
			}
			assert(ist_checkup(bpost)==true);
			ist_dispose(bpost);
			puts("end");
			if (finished==true)
				/* finished==true -> we hitted the bad states, the system is unsafe */
				retval=false;
			else {
				/* One more iteration is needed because both bpost and
				 * abs_post_star does not allow to conclude */
				printf("eec: BOUNDS\t");
				for(i=0;i<abs->nbV;printf("%d ",++abs->bound[i++]));
				printf("\n");
			}
		}
	}
	ist_dispose(downward_closed_initial_marking);

	times(&after);
	tick_sec = sysconf (_SC_CLK_TCK) ;
	comp_u = ((float)after.tms_utime - (float)before.tms_utime)/(float)tick_sec ;
	comp_s = ((float)after.tms_stime - (float)before.tms_stime)/(float)tick_sec ;
	printf("Total time of computation (user)   -> %6.3f sec.\n",comp_u);
	printf("                          (system) -> %6.3f sec.\n",comp_s);	

	if(retval == true)
		printf("retval = true\n");
	else
		printf("retval = false\n");	
	
	return retval;
}

//we compute pre^*, and stop the least fixpoint as soon as possible
void ic4pn_bis(system, initial_marking, bad) 
	transition_system_t *system;
	ISTSharingTree *bad, *initial_marking;
{
	abstraction_t *myabs, *newabs;
	transition_system_t *sysabs;
	ISTSharingTree *tmp,  *Z, *aZ, 
				   *alpha_initial_marking, *bad_iterates, *inter;
	size_t i,j,nb_iteration;
	boolean conclusive, eec_conclusive;

	//////////////////////////////////////////////////////////////////
	// creation of an abstraction that corresponds to the system    //
	// => to obtain stat.                                           //
	// ///////////////////////////////////////////////////////////////
	abstraction_t *systemabs;
	systemabs=(abstraction_t *)xmalloc(sizeof(abstraction_t));
	systemabs->nbConcreteV=system->limits.nbr_variables;
	systemabs->nbV=system->limits.nbr_variables;
	systemabs->bound=(integer16 *)xmalloc(systemabs->nbV*sizeof(integer16));
	systemabs->A=(integer16 **)xmalloc(systemabs->nbV*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i)
		systemabs->A[i]=(integer16 *)xmalloc(system->limits.nbr_variables*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i) {
		systemabs->bound[i]=1;
		for(j=0;j<system->limits.nbr_variables;++j)
			systemabs->A[i][j]=1;
	}
	/* printf("EEC for the concrete system\n");
	eec_conclusive=eec_fp(system,systemabs,initial_marking,bad,&lfp_eec);
	if (eec_conclusive == true)
		printf("Answer = true\n");
	else
		printf("Answer = false\n"); */
	printf("IC4PN..\n");

	bad_iterates = ist_copy(bad);

	////////////////////////////////////////////////////////////////////
	//end of eec for the concrete system                              //
	////////////////////////////////////////////////////////////////////
	
	tmp=ist_intersection(initial_marking,bad);
	conclusive = (ist_is_empty(tmp)==true ? false : true);
	ist_dispose(tmp);

	/* A_0 = refinement(Z_0) */
	Z = ist_copy(bad);
	ist_complement(Z,system->limits.nbr_variables);
	myabs=new_abstraction_dc_set(Z,system->limits.nbr_variables);

	printf("first abstraction\n");
	print_abstraction(myabs);

	ist_dispose(Z);

	nb_iteration=0;
	while(conclusive == false) {
		puts("begin of iteration");
		// We build the abstract system 
		sysabs=build_sys_using_abs(system,myabs);
		puts("The current abstraction is :");
		print_abstraction(myabs);
		// Set aZ, neg_aZ, alpha_initial_marking 
		aZ = ist_abstraction(bad_iterates,myabs);
		assert(ist_checkup(aZ)==true);
		alpha_initial_marking = ist_abstraction(initial_marking,myabs);
		assert(ist_checkup(alpha_initial_marking)==true);

		eec_conclusive=eec_fp_bis(sysabs,myabs,alpha_initial_marking, aZ);

		ist_dispose(aZ);

		if (eec_conclusive==true) {
			// says "safe" because it is indeed safe 
			puts("EEC concludes safe with the abstraction");
			print_abstraction(myabs);
			conclusive = true;
		} else { 
			printf("computation of a new iterates\n");
			tmp = ist_pre(bad_iterates,system);
			ISTSharingTree *tmp2 = ist_union(tmp,bad_iterates);
			ist_dispose(tmp);
			ist_dispose(bad_iterates);
			bad_iterates = ist_minimal_form(tmp2);

			printf("test for reachability\n");

			inter = ist_intersection(initial_marking,bad_iterates);
			if (ist_is_empty(inter) == false) {
				conclusive = true;
				printf("Reachable!\n");
			} else {


				newabs = new_abstraction_lub(bad_iterates,system->limits.nbr_variables,myabs);
				dispose_abstraction(myabs);
				myabs = newabs;

				printf("myabs\n");
				print_abstraction(myabs);
	
			}
			ist_dispose(inter);
		}
		// release sysabs 
		dispose_transition_system(sysabs);
		printf("end of iteration %d\n",++nb_iteration);
	}
}

//pre^*, we compute the least fixpoint and use its complement in the pre^* computation
void ic4pn_tris(system, initial_marking, bad) 
	transition_system_t *system;
	ISTSharingTree *bad, *initial_marking;
{
	abstraction_t *myabs, *newabs;
	transition_system_t *sysabs;
	ISTSharingTree *tmp,  *Z, *aZ, 
				   *alpha_initial_marking, *bad_iterates, *inter, *lfp, *neg_lfp;
	size_t i,j,nb_iteration;
	boolean conclusive, eec_conclusive;

	//////////////////////////////////////////////////////////////////
	// creation of an abstraction that corresponds to the system    //
	// => to obtain stat.                                           //
	// ///////////////////////////////////////////////////////////////
	abstraction_t *systemabs;
	systemabs=(abstraction_t *)xmalloc(sizeof(abstraction_t));
	systemabs->nbConcreteV=system->limits.nbr_variables;
	systemabs->nbV=system->limits.nbr_variables;
	systemabs->bound=(integer16 *)xmalloc(systemabs->nbV*sizeof(integer16));
	systemabs->A=(integer16 **)xmalloc(systemabs->nbV*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i)
		systemabs->A[i]=(integer16 *)xmalloc(system->limits.nbr_variables*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i) {
		systemabs->bound[i]=1;
		for(j=0;j<system->limits.nbr_variables;++j)
			systemabs->A[i][j]=1;
	}
	/* printf("EEC for the concrete system\n");
	eec_conclusive=eec_fp(system,systemabs,initial_marking,bad,&lfp_eec);
	if (eec_conclusive == true)
		printf("Answer = true\n");
	else
		printf("Answer = false\n"); */
	printf("IC4PN..\n");

	bad_iterates = ist_copy(bad);

	////////////////////////////////////////////////////////////////////
	//end of eec for the concrete system                              //
	////////////////////////////////////////////////////////////////////
	
	tmp=ist_intersection(initial_marking,bad);
	conclusive = (ist_is_empty(tmp)==true ? false : true);
	ist_dispose(tmp);

	/* A_0 = refinement(Z_0) */
	Z = ist_copy(bad);
	ist_complement(Z,system->limits.nbr_variables);
	myabs=new_abstraction_dc_set(Z,system->limits.nbr_variables);

	printf("first abstraction\n");
	print_abstraction(myabs);

	ist_dispose(Z);

	nb_iteration=0;
	while(conclusive == false) {
		puts("begin of iteration");
		// We build the abstract system 
		sysabs=build_sys_using_abs(system,myabs);
		puts("The current abstraction is :");
		print_abstraction(myabs);
		// Set aZ, neg_aZ, alpha_initial_marking 
		aZ = ist_abstraction(bad_iterates,myabs);
		assert(ist_checkup(aZ)==true);
		alpha_initial_marking = ist_abstraction(initial_marking,myabs);
		assert(ist_checkup(alpha_initial_marking)==true);

		eec_conclusive=eec_fp(sysabs,myabs,alpha_initial_marking, aZ,&lfp);

		ist_dispose(aZ);

		if (eec_conclusive==true) {
			// says "safe" because it is indeed safe 
			puts("EEC concludes safe with the abstraction");
			print_abstraction(myabs);
			conclusive = true;
		} else { 
			printf("computation of a new iterates\n");
			printf("%d %d\n",ist_nb_layers(lfp)-1,myabs->nbV);

			ist_complement(lfp,myabs->nbV);
			neg_lfp = ist_concretisation(lfp,myabs);
			ist_dispose(lfp);
			tmp = ist_union(neg_lfp,bad_iterates);
			ist_dispose(neg_lfp);
			ist_dispose(bad_iterates);
			bad_iterates = ist_minimal_form(tmp);
			tmp = ist_pre(bad_iterates,system);
			ISTSharingTree *tmp2 = ist_union(tmp,bad_iterates);
			ist_dispose(tmp);
			ist_dispose(bad_iterates);
			bad_iterates = ist_minimal_form(tmp2);

			printf("test for reachability\n");

			inter = ist_intersection(initial_marking,bad_iterates);
			if (ist_is_empty(inter) == false) {
				conclusive = true;
				printf("Reachable!\n");
			} else {


//				abs_tmp=new_abstraction(Z,system->limits.nbr_variables);

//				printf("abstraction new  method\n");
//				print_abstraction(new_abstraction_lub(Z,system->limits.nbr_variables,myabs));
				newabs = new_abstraction_lub(bad_iterates,system->limits.nbr_variables,myabs);

//				puts("abs_tmp");
//				print_abstraction(abs_tmp);
//				newabs = glb(abs_tmp,myabs);
				// release abs_tmp 
//				dispose_abstraction(abs_tmp);
				// release myabs
				dispose_abstraction(myabs);
				myabs = newabs;

				printf("myabs\n");
				print_abstraction(myabs);
	
			}
			ist_dispose(inter);
		}
		// release sysabs 
		dispose_transition_system(sysabs);
		printf("end of iteration %d\n",++nb_iteration);
	}
}


//pre^*, we stop least fixpoint as soon as possible, and we compute an abstract greatest fixpoint
void ic4pn_quatre(system, initial_marking, bad) 
	transition_system_t *system;
	ISTSharingTree *bad, *initial_marking;
{
	abstraction_t *myabs, *newabs;
	transition_system_t *sysabs;
	ISTSharingTree *tmp,  *Z, *aZ, 
				   *alpha_initial_marking, *bad_iterates_concrete, *inter, * frontier;
	size_t i,j,nb_iteration;
	boolean conclusive, eec_conclusive;
	int * trans;

	//////////////////////////////////////////////////////////////////
	// creation of an abstraction that corresponds to the system    //
	// => to obtain stat.                                           //
	// ///////////////////////////////////////////////////////////////
	abstraction_t *systemabs;
	systemabs=(abstraction_t *)xmalloc(sizeof(abstraction_t));
	systemabs->nbConcreteV=system->limits.nbr_variables;
	systemabs->nbV=system->limits.nbr_variables;
	systemabs->bound=(integer16 *)xmalloc(systemabs->nbV*sizeof(integer16));
	systemabs->A=(integer16 **)xmalloc(systemabs->nbV*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i)
		systemabs->A[i]=(integer16 *)xmalloc(system->limits.nbr_variables*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i) {
		systemabs->bound[i]=1;
		for(j=0;j<system->limits.nbr_variables;++j)
			systemabs->A[i][j]=1;
	}
	/* printf("EEC for the concrete system\n");
	eec_conclusive=eec_fp(system,systemabs,initial_marking,bad,&lfp_eec);
	if (eec_conclusive == true)
		printf("Answer = true\n");
	else
		printf("Answer = false\n"); */
	printf("IC4PN..\n");

	bad_iterates_concrete = ist_copy(bad);

	////////////////////////////////////////////////////////////////////
	//end of eec for the concrete system                              //
	////////////////////////////////////////////////////////////////////
	
	tmp=ist_intersection(initial_marking,bad);
	conclusive = (ist_is_empty(tmp)==true ? false : true);
	ist_dispose(tmp);

	/* A_0 = refinement(Z_0) */
	Z = ist_copy(bad);
	ist_complement(Z,system->limits.nbr_variables);
	myabs=new_abstraction_dc_set(Z,system->limits.nbr_variables);

	printf("first abstraction\n");
	print_abstraction(myabs);

	ist_dispose(Z);

	nb_iteration=0;
	while(conclusive == false) {
		puts("begin of iteration");
		// We build the abstract system 
		sysabs=build_sys_using_abs(system,myabs);
		puts("The current abstraction is :");
		print_abstraction(myabs);
		// Set aZ, neg_aZ, alpha_initial_marking 
		aZ = ist_abstraction(bad_iterates_concrete,myabs);
		ist_dispose(bad_iterates_concrete);
		assert(ist_checkup(aZ)==true);
		alpha_initial_marking = ist_abstraction(initial_marking,myabs);
		assert(ist_checkup(alpha_initial_marking)==true);

		eec_conclusive=eec_fp_bis(sysabs,myabs,alpha_initial_marking, aZ);

		if (eec_conclusive==true) {
			// says "safe" because it is indeed safe 
			puts("EEC concludes safe with the abstraction");
			print_abstraction(myabs);
			conclusive = true;
			ist_dispose(aZ);
		} else { 
			printf("computation of a new iterates\n");
			trans = list_of_exact_transitions(sysabs,myabs);
			tmp = pre_under_star(aZ,trans,sysabs,alpha_initial_marking);	
			ist_dispose(aZ);
			ist_dispose(alpha_initial_marking);

			if (tmp == NULL) {
				printf("In the abstract\n");
				conclusive = true;
				printf("Reachable\n");
			} else {
				frontier = ist_concretisation(tmp,myabs);
				ist_dispose(tmp);

	
				tmp = ist_pre(frontier,system);

				ISTSharingTree *tmp2 = ist_union(tmp,frontier);
				ist_dispose(tmp);
				ist_dispose(frontier);
				bad_iterates_concrete = ist_minimal_form(tmp2);

				printf("test for reachability\n");

				inter = ist_intersection(initial_marking,bad_iterates_concrete);
				if (ist_is_empty(inter) == false) {
					conclusive = true;
					printf("Reachable!\n");
				} else {
					newabs = new_abstraction_lub(bad_iterates_concrete,system->limits.nbr_variables,myabs);
					dispose_abstraction(myabs);
					myabs = newabs;

					printf("myabs\n");
					print_abstraction(myabs);
	
				}
				ist_dispose(inter);
			}
		}
		// release sysabs 
		dispose_transition_system(sysabs);
		printf("end of iteration %d\n",++nb_iteration);
	}
}

//pre^*, we use the complement of the least fixpoint in pre^*, and we compute an abstract greatest fixpoint
void ic4pn_cinq(system, initial_marking, bad) 
	transition_system_t *system;
	ISTSharingTree *bad, *initial_marking;
{
	abstraction_t *myabs, *newabs;
	transition_system_t *sysabs;
	ISTSharingTree *tmp,  *Z, *aZ, 
				   *alpha_initial_marking, *bad_iterates_concrete, *inter, * frontier, *lfp;
	size_t i,j,nb_iteration;
	boolean conclusive, eec_conclusive;
	int * trans;

	//////////////////////////////////////////////////////////////////
	// creation of an abstraction that corresponds to the system    //
	// => to obtain stat.                                           //
	// ///////////////////////////////////////////////////////////////
	abstraction_t *systemabs;
	systemabs=(abstraction_t *)xmalloc(sizeof(abstraction_t));
	systemabs->nbConcreteV=system->limits.nbr_variables;
	systemabs->nbV=system->limits.nbr_variables;
	systemabs->bound=(integer16 *)xmalloc(systemabs->nbV*sizeof(integer16));
	systemabs->A=(integer16 **)xmalloc(systemabs->nbV*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i)
		systemabs->A[i]=(integer16 *)xmalloc(system->limits.nbr_variables*sizeof(integer16));
	for(i=0;i<systemabs->nbV;++i) {
		systemabs->bound[i]=1;
		for(j=0;j<system->limits.nbr_variables;++j)
			systemabs->A[i][j]=1;
	}
	/* printf("EEC for the concrete system\n");
	eec_conclusive=eec_fp(system,systemabs,initial_marking,bad,&lfp_eec);
	if (eec_conclusive == true)
		printf("Answer = true\n");
	else
		printf("Answer = false\n"); */
	printf("IC4PN..\n");

	bad_iterates_concrete = ist_copy(bad);

	////////////////////////////////////////////////////////////////////
	//end of eec for the concrete system                              //
	////////////////////////////////////////////////////////////////////
	
	tmp=ist_intersection(initial_marking,bad);
	conclusive = (ist_is_empty(tmp)==true ? false : true);
	ist_dispose(tmp);

	/* A_0 = refinement(Z_0) */
	Z = ist_copy(bad);
	ist_complement(Z,system->limits.nbr_variables);
	myabs=new_abstraction_dc_set(Z,system->limits.nbr_variables);

	printf("first abstraction\n");
	print_abstraction(myabs);

	ist_dispose(Z);

	nb_iteration=0;
	while(conclusive == false) {
		puts("begin of iteration");
		// We build the abstract system 
		sysabs=build_sys_using_abs(system,myabs);
		puts("The current abstraction is :");
		print_abstraction(myabs);
		// Set aZ, neg_aZ, alpha_initial_marking 
		aZ = ist_abstraction(bad_iterates_concrete,myabs);
		ist_dispose(bad_iterates_concrete);
		assert(ist_checkup(aZ)==true);
		alpha_initial_marking = ist_abstraction(initial_marking,myabs);
		assert(ist_checkup(alpha_initial_marking)==true);

		eec_conclusive=eec_fp(sysabs,myabs,alpha_initial_marking, aZ,&lfp);

		if (eec_conclusive==true) {
			// says "safe" because it is indeed safe 
			puts("EEC concludes safe with the abstraction");
			print_abstraction(myabs);
			conclusive = true;
			ist_dispose(aZ);
			ist_dispose(lfp);
		} else { 
			printf("computation of a new iterates\n");

			ist_complement(lfp,myabs->nbV);
			tmp = ist_union(lfp,aZ);
			ist_dispose(aZ);
			ist_dispose(lfp);
			aZ = ist_minimal_form(tmp);

			trans = list_of_exact_transitions(sysabs,myabs);
			tmp = pre_under_star(aZ,trans,sysabs,alpha_initial_marking);	
			ist_dispose(aZ);
			ist_dispose(alpha_initial_marking);

			if (tmp == NULL) {
				printf("In the abstract\n");
				conclusive = true;
				printf("Reachable\n");
			} else {
				frontier = ist_concretisation(tmp,myabs);
				ist_dispose(tmp);

	
				tmp = ist_pre(frontier,system);

				ISTSharingTree *tmp2 = ist_union(tmp,frontier);
				ist_dispose(tmp);
				ist_dispose(frontier);
				bad_iterates_concrete = ist_minimal_form(tmp2);

				printf("test for reachability\n");

				inter = ist_intersection(initial_marking,bad_iterates_concrete);
				if (ist_is_empty(inter) == false) {
					conclusive = true;
					printf("Reachable!\n");
				} else {
					newabs = new_abstraction_lub(bad_iterates_concrete,system->limits.nbr_variables,myabs);
					dispose_abstraction(myabs);
					myabs = newabs;

					printf("myabs\n");
					print_abstraction(myabs);
	
				}
				ist_dispose(inter);
			}
		}
		// release sysabs 
		dispose_transition_system(sysabs);
		printf("end of iteration %d\n",++nb_iteration);
	}
}

int main(int argc, char *argv[ ])
{
	T_PTR_tree atree;
	transition_system_t *system;
	ISTSharingTree *initial_marking, *bad;

	head_msg();
	mist_cmdline_options_handle(argc, argv);

	linenumber = 1;
	tbsymbol_init(&tbsymbol, 4096);

	printf("\n\n"); 
	printf("Parsing the problem instance.\n");

	my_yyparse(&atree, argv[optind++]);

	/* We initialize the memory management of the system (must do it before parsing) */
	printf("Allocating memory for data structure.. ");
	ist_init_system();
	printf("DONE\n");

#ifdef TBSYMB_DUMP  
	printf("\n\n");
	tbsymbol_dump(tbsymbol, &callback);
#endif    

#ifdef TREE_DUMP  
	printf("\n\n");
	tree_dump(atree, callback_tree_before, callback_tree_after, callback_leaf);
#endif

		
	build_problem_instance(atree, &system, &initial_marking, &bad);
	printf("System has %3d variables, %3d transitions and %2d actual invariants\n",system->limits.nbr_variables, system->limits.nbr_rules, system->limits.nbr_invariants);

	//backward_reachability(system,initial_marking,bad);
	//ic4pn(system,initial_marking,bad);
	//cegar(system,initial_marking,bad);
	//ic4pn_bis(system,initial_marking,bad);
	//ic4pn_tris(system,initial_marking,bad);
	//ic4pn_quatre(system,initial_marking,bad);
	ic4pn_cinq(system,initial_marking,bad);

	ist_dispose(initial_marking);
	ist_dispose(bad);
	dispose_transition_system(system);

	tbsymbol_destroy(&tbsymbol);

	puts("Thanks for using this tool");

	return 0;
}
