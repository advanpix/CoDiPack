/**
 * CoDiPack, a Code Differentiation Package
 *
 * Copyright (C) 2015 Chair for Scientific Computing (SciComp), TU Kaiserslautern
 * Homepage: http://www.scicomp.uni-kl.de
 * Contact:  Prof. Nicolas R. Gauger (codi@scicomp.uni-kl.de)
 *
 * Lead developers: Max Sagebaum, Tim Albring (SciComp, TU Kaiserslautern)
 *
 * This file is part of CoDiPack (http://www.scicomp.uni-kl.de/software/codi).
 *
 * CoDiPack is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * CoDiPack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU
 * General Public License along with CoDiPack.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Max Sagebaum, Tim Albring, (SciComp, TU Kaiserslautern)
 *          Prof. Robin Hogan, (Univ. of Reading).
 *
 * Originally based on Adept 1.0 (http://www.met.rdg.ac.uk/clouds/adept/)
 * released under GPL 3.0 (Copyright (C) 2012-2013 Robin Hogan and the University of Reading).
 */

/*
 * In order to include this file the user has to define the preprocessor macro NAME, FUNCTION and PRIMAL_FUNCTION.
 * NAME contains the name of the generated operation. FUNCTION represents the normal name of that function
 * e.g. 'operator -' or 'sin'. PRIMAL_FUNCTION is the name of a function which can call the primal operator.
 *
 * The defines NAME, FUNCTION and PRIMAL_FUNCTION will be undefined at the end of this template.
 *
 * The user needs to define further the following functions:
 *
 * gradName
 */

#ifndef NAME
  #error Please define a name for the binary expression.
#endif
#ifndef FUNCTION 
  #error Please define the primal function representation.
#endif
#ifndef PRIMAL_FUNCTION 
  #error Please define a function which calls the primal functions representation.
#endif

#define COMBINE2(A,B) A ## B
#define COMBINE(A,B) COMBINE2(A,B)

#define OP NAME
#define FUNC FUNCTION
#define PRIMAL_CALL PRIMAL_FUNCTION
#define GRADIENT_FUNC   COMBINE(grad, NAME)

/* predefine the struct and the function for higher order derivatives */
template<typename Real, class A> struct OP;

template <typename Real, class A>
inline  codi:: OP<Real, A> FUNC(const codi::Expression<Real, A>& a);

/** 
 * @brief Expression implementation for OP.
 *
 * @tparam Real  The real type used in the active types.
 * @tparam    A  The expression for the argument of the function.
 */
template<typename Real, class A>
struct OP : public Expression<Real, OP<Real, A> > {
  private:
    /** @brief The argument of the function. */
    const A a_;
    /** @brief The result of the function. It is always precomputed. */
    Real result_;
  public:
    /**
     * @brief The passive type used in the origin.
     *
     * If Real is not an ActiveReal this value corresponds to Real,
     * otherwise the PassiveValue from Real is used.
     */
    typedef typename TypeTraits<Real>::PassiveReal PassiveReal;

    /** 
     * @brief Stores the argument of the expression.
     *
     * @param[in] a Argument of the expression.
     */
    OP(const Expression<Real, A>& a) :
      a_(a.cast()),
      result_(PRIMAL_CALL(a.getValue())) {}

  /** 
   * @brief Calculates the jacobie of the expression and hands them down to the argument.
   *
   * For f(x) it calculates df/dx and passes this value as the multiplier to the argument.
   *
   * @param[inout] data A helper value which the tape can define and use for the evaluation.
   *
   * @tparam Data The type for the tape data.
   */
  template<typename Data>
  inline void calcGradient(Data& data) const {
    a_.calcGradient(data, GRADIENT_FUNC(a_.getValue(), result_));
  }

  /** 
   * @brief Calculates the jacobi of the expression and hands them down to the argument. 
   *
   * For f(x) it calculates multiplier * df/dx and passes this value as the multiplier to the argument.
   *
   * @param[inout]     data A helper value which the tape can define and use for the evaluation.
   * @param[in]  multiplier The Jacobi from the expression where this expression was used as an argument.
   *
   * @tparam Data The type for the tape data.
   */
  template<typename Data>
  inline void calcGradient(Data& data, const Real& multiplier) const {
    a_.calcGradient(data, GRADIENT_FUNC(a_.getValue(), result_)*multiplier);
  }

  /** 
   * @brief Return the numerical value of the expression. 
   *
   * @return The value of the expression. 
   */
  inline const Real& getValue() const {
    return result_;
  }

  template<typename NewActiveType, typename NewGradientData, size_t activeOffset, size_t passiveOffset>
  static inline decltype(auto) exchangeActiveType(const Real* primalValues, const NewGradientData* gradientData, const PassiveReal* passiveValues) {
    return OP< Real,
        decltype(A::template exchangeActiveType<NewActiveType, NewGradientData, activeOffset, passiveOffset>(primalValues, gradientData, passiveValues))
        > (
        A::template exchangeActiveType<NewActiveType, NewGradientData, activeOffset, passiveOffset>(primalValues, gradientData, passiveValues)
      );
  }

  template<typename Data, typename IndexType, typename NewActiveType, typename NewGradientData >
  static void evalAdjoint2(Data& gradient, const Real& seed, const Real* primalValues, const NewGradientData* gradientData, const PassiveReal* passiveValues) {
    auto newExpr = exchangeActiveType<NewActiveType, NewGradientData, 0, 0> (primalValues, gradientData, passiveValues);
    newExpr.calcGradient(gradient, seed);
  }

  template<typename IndexType, size_t offset, size_t passiveOffset>
  static inline Real getValue(const IndexType* indices, const PassiveReal* passiveValues, const Real* primalValues) {
    const Real aPrimal = A::template getValue<IndexType, offset, passiveOffset>(indices, passiveValues, primalValues);

    return FUNC(aPrimal);
  }

  template<typename IndexType>
  static void evalAdjoint(const Real& seed, const IndexType* indices, const PassiveReal* passiveValues, const Real* primalValues, Real* adjointValues) {
    evalAdjointOffset<IndexType, 0, 0>(seed, indices, primalValues, adjointValues);
  }

  template<typename IndexType, size_t offset, size_t passiveOffset>
  static inline void evalAdjointOffset(const Real& seed, const IndexType* indices, const PassiveReal* passiveValues, const Real* primalValues, Real* adjointValues) {
    const Real aPrimal = A::template getValue<IndexType, offset, passiveOffset>(indices, passiveValues, primalValues);
    const Real resPrimal = FUNC(aPrimal);

    const Real aJac = DERIVATIVE_FUNC(aPrimal, resPrimal) * seed;
    A::template evalAdjointOffset<IndexType, offset, passiveOffset>(aJac, indices, passiveValues, primalValues, adjointValues);
  }

  inline void pushPassive(const PassiveReal& passive) const {
    a_.pushPassive(passive);
  }
};

/** 
 * @brief Overload for FUNC with the CoDiPack expressions.
 *
 * @param[in] a The argument of the operation.
 *
 * @return The implementing expression OP.
 *
 * @tparam Real The real type used in the active types.
 * @tparam A The expression for the first argument of the function.
 */
template <typename Real, class A>
inline codi:: OP<Real, A> FUNC(const codi::Expression<Real, A>& a) {
  return codi:: OP<Real, A>(a.cast());
}

#undef OP
#undef FUNC
#undef PRIMAL_CALL
#undef GRADIENT_FUNC
#undef COMBINE
#undef COMBINE2

#undef PRIMAL_FUNCTION
#undef FUNCTION
#undef NAME
