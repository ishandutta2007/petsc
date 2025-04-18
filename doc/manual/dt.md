(ch_dt)=

# PetscDT: Discretization Technology in PETSc

This chapter discusses the low-level infrastructure which supports higher-level discretizations in PETSc, which includes things such as quadrature and probability distributions.

## Quadrature

## Probability Distributions

A probability distribution function (PDF) returns the probability density at a given location $P(x)$, so that the probability for an event at location in $[x, x+dx]$ is $P(x) dx$. This means that we must have the normalization condition,

$$
\int_\Omega P(x) dx = 1.
$$

where :math:Omega is the domain for $x$. This requires that the PDF must have units which are the inverse of the volume form $dx$, meaning that it is homogeneous of order $d$ under scaling

$$
pdf(x) = \lambda^d P(\lambda x).
$$

We can check this using the normalization condition,

$$
\begin{aligned}
  \int_\Omega P(x) dx &= \int_\Omega P(\lambda s) \lambda^d ds \\
                      &= \int_\Omega P(s) \lambda^{-d} \lambda^d ds \\
                      &= \int_\Omega P(s) ds \\
                      &= 1
\end{aligned}
$$

The cumulative distribution function (CDF) is the incomplete integral of the PDF,

$$
C(x) = \int^x_{x_-} P(s) ds
$$

where $x_-$ is the lower limit of our domain. We can work out the effect of scaling on the CDF using this definition,

$$
\begin{aligned}
  C(\lambda x) &= \int^{\lambda x}_{x_-} P(s) ds \\
               &= \int^{x}_{x_-} \lambda^d P(\lambda t) dt \\
               &= \int^{x}_{x_-} P(t) dt \\
               &= C(x)
\end{aligned}
$$

so the CDF itself is scale invariant and unitless.

We do not add a scale argument to the PDF in PETSc, since all variables are assuming to be dimensionless. This means that inputs to the PDF and CDF should be scaled by the appropriate factor for the units of $x$, and the output can be rescaled if it is used outside the library.
