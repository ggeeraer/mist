// vim:sw=4:ts=4
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

   Copyright 2006, Pierre Ganty
 */

#include"abstraction.h"
#include"xmalloc.h"

/* Only works for Petri Nets w/o invariants */
transition_system_t *
build_sys_using_abs(sys,abs)
	transition_system_t *sys;
	abstraction_t *abs;
{
	transition_system_t *retval;
	size_t i,j,k;
	
	retval=(transition_system_t *)xmalloc(sizeof(transition_system_t));
	retval->limits.nbr_rules = sys->limits.nbr_rules;
	retval->limits.nbr_variables = abs->nbV;
	/* Assignment for sanity of data structure */
	retval->limits.nbr_invariants=0;
	retval->transition=(transition_t *)xmalloc(retval->limits.nbr_rules*sizeof(transition_t));
	for(i=0;i<retval->limits.nbr_rules;++i){
		retval->transition[i].cmd_for_place = (gd_command_t *)xmalloc(abs->nbV*sizeof(gd_command_t));
		/* Assignment for sanity of data structure */
		retval->transition[i].nbr_transfers=0;
		for(j=0;j<retval->limits.nbr_variables;++j){
			/* We maintain for each place in the abstract net how many concrete ones it represent */
			retval->transition[i].cmd_for_place[j].places_abstracted=0;
			/* Start from bottom values for delta and guard */
			retval->transition[i].cmd_for_place[j].delta =0L;
			ist_assign_values_to_interval(&retval->transition[i].cmd_for_place[j].guard, 0L,  INFINITY);
			for(k=0;k<sys->limits.nbr_variables;++k){
				if(abs->A[j][k]>0){
					retval->transition[i].cmd_for_place[j].delta+= sys->transition[i].cmd_for_place[k].delta;
					ist_add_value_to_interval(&retval->transition[i].cmd_for_place[j].guard, sys->transition[i].cmd_for_place[k].guard.Left);
					++retval->transition[i].cmd_for_place[j].places_abstracted;
				}
			}
		}
	}
	return retval;	
}

boolean is_layer_unbounded(layer)
	ISTLayer *layer;
{
	ISTNode *Node=layer->FirstNode;
	while(Node!=NULL){
		if(ist_is_unbounded(Node->Info)==true) {
			return true;
		}
		Node=Node->Next;
	}
	return false;
}

/* We assume exists i s.t. sum cur_abs->A[i][j] > 1 */
abstraction_t *refine_abs(system, cur_abs, S)
	transition_system_t *system;
	abstraction_t *cur_abs;
	ISTSharingTree *S;
{
	size_t i,j,current,first_non_singleton_row;
	boolean bef_1st_non0_occur,first_bound,split;
	abstraction_t *retval;
	ISTLayer *layer;
	retval=(abstraction_t *)xmalloc(sizeof(abstraction_t));
	/* A refinement step add exactly one new dimension w.r.t. the current abstraction */
	retval->nbV=(cur_abs->nbV+1);
	retval->A=(integer16 **)xmalloc(retval->nbV*sizeof(integer16));
	retval->bound=(integer16 *)xmalloc(retval->nbV*sizeof(integer16));
	for(i=0;i<retval->nbV;++i){
		retval->A[i]=(integer16 *)xmalloc(system->limits.nbr_variables*sizeof(integer16));
		/* The new abstraction is initialized w/ 0's everywhere */
		for(j=0;j<system->limits.nbr_variables;++j)
			retval->A[i][j]=0;
		retval->bound[i]=1;
	}
	/* current : pointer in retval, i : pointer in cur_abs */
	for(current=0,i=0;i<cur_abs->nbV && current<retval->nbV;++i,++current){
		/* Did we add a new line ? */
		split=false;
		/* Is i before the first non zero occurence ? */
		bef_1st_non0_occur=true;
		/* We refine according to the value in S */
		layer = S->FirstLayer;
		for(j=0;j<system->limits.nbr_variables;++j){
			if(cur_abs->A[i][j] == 1) {
				/* By default we copy the line */
				retval->A[current][j]=1;
				/* if haven't added a new line yet */
				if(current == i){
					if (bef_1st_non0_occur==false){
						if( is_layer_unbounded(layer) != first_bound ){
							/* We add a new line. We split unbounded from bounded places in cur_abs->A[i] */
							split=true;
							retval->A[current][j]=0;
							retval->A[current+1][j]=1;
						}
						first_non_singleton_row=i;

					} else {
						first_bound = is_layer_unbounded(layer);
						bef_1st_non0_occur=false;
					}
				}
			}
			layer=layer->Next;
		}
		if(split == true)
			++current;

	}
	if (current==i){
		for(j=0;cur_abs->A[first_non_singleton_row][j]==0;++j);
		retval->A[first_non_singleton_row][j]=0;
		retval->A[current][j]=1;
	}
	/* We release the current_abstraction */
	for(i=0;i<cur_abs->nbV;++i)
		free(cur_abs->A[i]);
	free(cur_abs->bound);
	free(cur_abs);
	return retval;
}

ISTSharingTree *ist_abstract_post_of_rules(ISTSharingTree * S, abstraction_t * abs, transition_system_t *t, int rule) 
{
	ISTSharingTree * result;
	ISTLayer * L;
	ISTNode * N;
	ISTInterval * v;
	int i;
	
	result =  ist_copy(S);	
	//intersection with guard
	//If the info of a node has no intersection with guard, we remove its sons, i.e. the node 
	//becomes useless and will be removed later.	
	for (i = 0, L = result->FirstLayer; i < abs->nbV; i++,L = L->Next) {
		N = L->FirstNode;
	    	while (N != NULL) {
			v = ist_intersect_intervals(N->Info,&t->transition[rule].cmd_for_place[i].guard);
			if (v == NULL) {
				ist_remove_sons(N);
				ist_dispose_info(v);
			} else {
				ist_dispose_info(N->Info);
				N->Info = v;
			}
			N = N->Next;
		}
	}

	//we remove the useless nodes
	ist_remove_node_without_son(result);

	//If the IST is not empty, we apply the effect of the function
	if (ist_is_empty(result) == false) {
		for (i = 0, L = result->FirstLayer; i < abs->nbV; i++, L = L->Next) {
			N = L->FirstNode;
	    		while (N != NULL) {
				if ((N->Info->Right != INFINITY) &&
					(N->Info->Right + t->transition[rule].cmd_for_place[i].delta <=
					abs->bound[i]))
					N->Info->Right = N->Info->Right + t->transition[rule].cmd_for_place[i].delta;
				else
					N->Info->Right = INFINITY;
				N = N->Next;
			}
		}	
	}
	return result;
}


/*
 * apply the (0,...,k,INFINITY) abstraction
 */

void abstract_bound(ISTSharingTree *S, abstraction_t * abs) 
{
	ISTLayer * L;
	ISTNode * N;
	int i;

	for(L = S->FirstLayer, i = 0 ; L != S->LastLayer ; L = L->Next, i++) 
		for(N = L->FirstNode ; N != NULL ; N = N->Next) 
			if (N->Info->Right > abs->bound[i])
				ist_assign_values_to_interval(N->Info,N->Info->Left,INFINITY);
}


/*
 * function that add tuples that are lesser than tuples in S
 */


void ist_downward_closure(ISTSharingTree * S)
{
	ISTLayer *L;
	ISTNode *N;

	for(L = S->FirstLayer;L < S->LastLayer;L = L->Next)
		for(N = L->FirstNode; N != NULL;N = N->Next) 
			ist_assign_values_to_interval(N->Info,0,N->Info->Right);
	
}


/*
 * compute the pretild for one transition t for the abstract system
 */
ISTSharingTree * abstract_place_pretild_rule(ISTSharingTree * S, abstraction_t * abs, transition_system_t *t, int rule) 
{
	ISTSharingTree * result = NULL;
	ISTSharingTree * temp;
	ISTLayer * L;
	int i;
	boolean top;
	for (i=0,L = S->FirstLayer, top = false; (i < abs->nbV) && (top == false);i++,L = L->Next) {
		if ((t->transition[rule].cmd_for_place[i].guard.Left > 0) && 
		(t->transition[rule].cmd_for_place[i].places_abstracted > 1)) {
			top = true;
		}
	}
	if (top == false) {
		temp = ist_copy(S);
		ist_complement(temp,abs->nbV);
		result = ist_pre_of_transfer(temp, &t->transition[rule], abs->nbV);
		ist_dispose(temp);
		ist_complement(result,abs->nbV);
	}
	else {
		ist_new(&result);
		ist_complement(result,abs->nbV);
	}
	return result;	
}


/*
 * compute the abstrat pretild for the abstract system for all transitions
 *
 */

ISTSharingTree * abstract_place_pretild(ISTSharingTree * S, abstraction_t * abs, transition_system_t *t)
{
	ISTSharingTree * result;
	ISTSharingTree * temp1;
	ISTSharingTree * temp2;
	int i;

	ist_new(&result);
	ist_complement(result,abs->nbV);
	for(i = 0; i < t->limits.nbr_rules; i++) {
		temp1 = abstract_place_pretild_rule(S,abs,t,i);
		temp2 = ist_intersection(result,temp1);
		ist_dispose(result);
		ist_dispose(temp1);
		result= temp2;
	}
	
	return result;
}

/*
 * compute the abstract pretild
 *
 */

ISTSharingTree *abstract_pretild(ISTSharingTree * S, abstraction_t * abs, transition_system_t *t)
{
	ISTSharingTree * result;
	result = abstract_place_pretild(S,abs,t);
	abstract_bound(result,abs);
	return result;	
}
