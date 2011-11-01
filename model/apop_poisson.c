/* The Poisson distribution.
 Copyright (c) 2006--2007, 2010 by Ben Klemens.  Licensed under the modified GNU GPL v2; see COPYING and COPYING2.  
  
\amodel apop_poisson The Poisson distribution.

\f$p(k) = {\mu^k \over k!} \exp(-\mu), \f$

\adoc    Input_format  Location of data in the grid is not relevant; send it a 1 x N, N x 1, or N x M and it will all be the same.
\adoc    Parameter_format  One parameter, the zeroth element of the vector.    
\adoc    settings   \ref apop_parts_wanted_settings, for the \c .want_cov element.  */

#include "apop_internal.h"

static double apply_me(double x, void *in){
  double    *ln_l = in;
    return x==0 ? 0 :  *ln_l *x - gsl_sf_lngamma(x+1);
}

static double poisson_log_likelihood(apop_data *d, apop_model * p){
  Nullcheck_mpd(d, p)
  Get_vmsizes(d) //tsize
  double lambda = gsl_vector_get(p->parameters->vector, 0);
  double ln_l 	= log(lambda);
  double ll     = apop_map_sum(d, .fn_dp = apply_me, .param=&ln_l);
    return ll - tsize*lambda;
}

static double data_mean(apop_data *d){
    Get_vmsizes(d)
    if (vsize && !msize1) return apop_vector_mean(d->vector);
    if (!vsize && msize1) return apop_matrix_mean(d->matrix);
    return apop_matrix_mean(d->matrix)*(msize1*msize2)/tsize +
               + apop_vector_mean(d->vector)*vsize/tsize;
}

/* \adoc estimated_parameters 
Unless you decline it by adding the \ref apop_parts_wanted_settings group, I will also give you the variance of the parameter, via bootstrap, stored in a page named <tt>\<Covariance\></tt>.

\adoc estimated_info   Reports <tt>log likelihood</tt>. */
static apop_model * poisson_estimate(apop_data * data,  apop_model *est){
  Nullcheck_mpd(data, est);
  double mean = data_mean(data);
	gsl_vector_set(est->parameters->vector, 0, mean);
    apop_data_add_named_elmt(est->info, "log likelihood", poisson_log_likelihood(data, est));
    //to prevent an infinite loop, the bootstrap needs to be flagged to not run itself. 
    apop_parts_wanted_settings *p = apop_settings_get_group(est, apop_parts_wanted);
    if (!p || p->covariance=='y'){
        if (!p) Apop_model_add_group(est, apop_parts_wanted);
        else    p->covariance='n';

        apop_data_add_page(est->parameters, 
                apop_bootstrap_cov(data, *est), 
                "<Covariance>");

        if (!p) Apop_settings_rm_group(est, apop_parts_wanted);
        else    p->covariance='y';
    }
	return est;
}

static double positive_beta_constraint(apop_data *returned_beta, apop_model *v){
    //constraint is 0 < beta_1
    return apop_linear_constraint(v->parameters->vector, .margin = 1e-4);
}

static void poisson_dlog_likelihood(apop_data *d, gsl_vector *gradient, apop_model *p){
    Get_vmsizes(d) //tsize
    Nullcheck_mpd(d, p)
    double     lambda = gsl_vector_get(p->parameters->vector, 0);
    gsl_matrix *data = d->matrix;
    double     d_a = apop_matrix_sum(data)/lambda - tsize;
    gsl_vector_set(gradient,0, d_a);
}

/* \adoc RNG Just a wrapper for \c gsl_ran_poisson.  */
static void poisson_rng(double *out, gsl_rng* r, apop_model *p){
    *out = gsl_ran_poisson(r, *p->parameters->vector->data);
}

apop_model apop_poisson = {"Poisson distribution", 1, 0, 0, .dsize=1,
     .estimate = poisson_estimate, .log_likelihood = poisson_log_likelihood, 
     .score = poisson_dlog_likelihood, .constraint = positive_beta_constraint, 
     .draw = poisson_rng};
