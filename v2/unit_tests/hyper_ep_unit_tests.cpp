#include <cmath>

#include <lgr_hyper_ep.hpp>
#include "lgr_gtest.hpp"
#include <Teuchos_ParameterList.hpp>

namespace {

using scalar_type = double;
namespace Details = lgr::HyperEPDetails;
using tensor_type = Details::tensor_type;

#ifdef OMEGA_H_THROW
using invalid_argument = Omega_h::exception;
#else
using invalid_argument = std::invalid_argument;
#endif

namespace hyper_ep_utils {

static scalar_type copper_density() { return 8930.; }

static std::vector<scalar_type> copper_johnson_cook_props()
{
  // Properties
  scalar_type youngs_modulus = 200.e9;
  scalar_type poisson_ratio = 0.333;

  // Johnson cook hardening
  scalar_type ajo = 8.970000E+08;
  scalar_type bjo = 2.918700E+09;
  scalar_type njo = 3.100000E-01;

  // Temperature dependence
  scalar_type tjo = 1.189813E-01;
  scalar_type tmo = std::numeric_limits<scalar_type>::max();
  scalar_type mjo = 1.090000E+00;

  // Rate dependence
  scalar_type cjo = 2.500000E-02  ;
  scalar_type epdot0 = 1.;

  std::vector<scalar_type> props {youngs_modulus, poisson_ratio,
                                  ajo, bjo, njo, tjo, tmo, mjo, cjo, epdot0};

  return props;
}


static std::vector<scalar_type> copper_zerilli_armstrong_props()
{
  // Properties
  scalar_type youngs_modulus = 200.e9;
  scalar_type poisson_ratio = 0.333;

  // Constant yield strength
  scalar_type a = 6.500000E+08;

  // Power law hardening
  scalar_type b = 0.000000E+00;
  scalar_type n = 1.000000E+00;

  //
  scalar_type c1 = 0.000000E+00;
  scalar_type c2 = 8.900000E+09;
  scalar_type c3 = 3.249400E+01;

  // Rate dependence
  scalar_type c4 = 1.334575E+00;

  std::vector<scalar_type> props {youngs_modulus, poisson_ratio,
                                  a, b, n, c1, c2, c3, c4};

  return props;
}


static void eval_prescribed_motions(
    const scalar_type eps,
    const Details::Elastic& elastic,
    const Details::Hardening& hardening,
    const Details::RateDependence& rate_dep,
    const std::vector<scalar_type>& props,
    const scalar_type& rho)
{

  scalar_type youngs_modulus = props[0];
  scalar_type poisson_ratio = props[1];
  scalar_type bulk_modulus = youngs_modulus/3./(1.-2.*poisson_ratio);
  scalar_type shear_modulus = youngs_modulus/2./(1.+poisson_ratio);
  scalar_type wave_speed_expected = std::sqrt((bulk_modulus+(4./3.)*shear_modulus)/rho);

  scalar_type dtime = 1.;
  scalar_type temp = 298.;

  // Initialize in/out variables to be updated by material update
  tensor_type T;
  tensor_type F = Omega_h::identity_matrix<3, 3>();
  tensor_type Fp = Omega_h::identity_matrix<3, 3>();

  scalar_type wave_speed = 0.;
  scalar_type ep = 0.;
  scalar_type epdot = 0.;

  Details::ErrorCode err;

  // Uniaxial strain, tension
  F(0,0) = 1. + eps; F(0,1) = 0.;       F(0,2) = 0.;
  F(1,0) = 0.;       F(1,1) = 1.;       F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1.;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "UNIAXIAL STRAIN, TENSION EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";
  EXPECT_TRUE(Omega_h::are_close(wave_speed, wave_speed_expected))
    << "EXPECTED WAVE SPEED: " << wave_speed_expected << ", "
    << "CALCULATED WAVE SPEED: " << wave_speed;

  // Uniaxial strain, compression
  F(0,0) = 1. - eps; F(0,1) = 0.;       F(0,2) = 0.;
  F(1,0) = 0.;       F(1,1) = 1.;       F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1.;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "UNIAXIAL STRAIN, COMPRESSION EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

  // Simple shear, 2D
  F(0,0) = 1.;       F(0,1) = eps;      F(0,2) = 0.;
  F(1,0) = 0.;       F(1,1) = 1.;       F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1.;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "SIMPLE SHEAR, 2D EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

  // Hydrostatic compression
  F(0,0) = 1. - eps; F(0,1) = 0.;       F(0,2) = 0.;
  F(1,0) = 0.;       F(1,1) = 1. - eps; F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1. - eps;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "HYDROSTATIC COMPRESSION EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

  // Hydrostatic tension
  F(0,0) = 1. + eps; F(0,1) = 0.;       F(0,2) = 0.;
  F(1,0) = 0.;       F(1,1) = 1. + eps; F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1. + eps;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "HYDROSTATIC TENSION EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

  // Simple shear, 3D
  F(0,0) = 1.;       F(0,1) = eps;      F(0,2) = 0.;
  F(1,0) = eps;      F(1,1) = 1.;       F(1,2) = eps;
  F(2,0) = eps;      F(2,1) = 0.;       F(2,2) = 1.;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "SIMPLE SHEAR, 3D EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

  // Biaxial strain, tension
  F(0,0) = 1. + eps; F(0,1) = 0.;       F(0,2) = 0.;
  F(1,0) = 0.;       F(1,1) = 1. + eps; F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1.;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "BIAXIAL STRAIN, TENSION EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

  // Biaxial strain, compression
  F(0,0) = 1. - eps; F(0,1) = 0.;       F(0,2) = 0.;
  F(1,0) = 0.;       F(1,1) = 1. - eps; F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1.;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "BIAXIAL STRAIN, COMPRESSION EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

  // Pure shear, 2D
  F(0,0) = 1.;       F(0,1) = eps;      F(0,2) = 0.;
  F(1,0) = eps;      F(1,1) = 1.;       F(1,2) = 0.;
  F(2,0) = 0.;       F(2,1) = 0.;       F(2,2) = 1.;
  err = Details::eval(elastic, hardening, rate_dep, props, rho,
                      F, dtime, temp, T, wave_speed, Fp, ep, epdot);
  EXPECT_TRUE(err == Details::ErrorCode::SUCCESS)
    << "PURE SHEAR, 2D EVAL FAILED WITH ERROR "
    << "'" << Details::get_error_code_string(err) << "'";

}
} // namespace hyper_ep_utils



TEST(HyperEPMaterialModel, ParameterValidation)
{
  using Teuchos::ParameterList;
  scalar_type tol = 1e-14;

  { // Elastic
    auto p = ParameterList("elastic");
    p.set<scalar_type>("E", 10.);
    p.set<scalar_type>("Nu", .1);
    {
      auto params = ParameterList("model");
      params.set("elastic", p);
      std::vector<scalar_type> props;
      Details::Elastic elastic;
      Details::read_and_validate_elastic_params(params, props, elastic);
      EXPECT_TRUE(props.size() == 2);
      EXPECT_TRUE(std::abs(props[0] - 10.) < tol);
      EXPECT_TRUE(std::abs(props[1] - .1) < tol);
      EXPECT_TRUE(elastic == Details::Elastic::LINEAR_ELASTIC);
    }

    {
      p.set<std::string>("hyperelastic", "neo hookean");
      auto params = ParameterList("model");
      params.set("elastic", p);
      std::vector<scalar_type> props;
      Details::Elastic elastic;
      Details::read_and_validate_elastic_params(params, props, elastic);
      EXPECT_TRUE(props.size() == 2);
      EXPECT_TRUE(std::abs(props[0] - 10.) < tol);
      EXPECT_TRUE(std::abs(props[1] - .1) < tol);
      EXPECT_TRUE(elastic == Details::Elastic::NEO_HOOKEAN);
    }
  }

  { // Plastic
    auto p0 = ParameterList("plastic");
    p0.set<scalar_type>("A", 10.);
    p0.set<scalar_type>("B", 2.);
    p0.set<scalar_type>("N", .1);
    p0.set<scalar_type>("T0", 400.);
    p0.set<scalar_type>("TM", 500.);
    p0.set<scalar_type>("M", .2);

    auto p1 = ParameterList("rate dependent");
    p1.set<std::string>("type", "johnson cook");
    p1.set<scalar_type>("C", 5.0);
    p1.set<scalar_type>("EPDOT0", 1.0);

    {
      // Von Mises
      auto params = ParameterList("model");
      params.set("plastic", p0);
      std::vector<scalar_type> props;
      Details::Hardening hardening;
      Details::RateDependence rate_dep;
      Details::read_and_validate_plastic_params(params, props, hardening, rate_dep);
      EXPECT_TRUE(props.size() == 8);
      EXPECT_TRUE(std::abs(props[0] - 10.) < tol);
      EXPECT_TRUE(hardening == Details::Hardening::NONE);
      EXPECT_TRUE(rate_dep == Details::RateDependence::NONE);
    }

    {
      // Isotropic hardening
      p0.set<std::string>("hardening", "linear isotropic");
      auto params = ParameterList("model");
      params.set("plastic", p0);
      std::vector<scalar_type> props;
      Details::Hardening hardening;
      Details::RateDependence rate_dep;
      Details::read_and_validate_plastic_params(params, props, hardening, rate_dep);
      EXPECT_TRUE(props.size() == 8);
      EXPECT_TRUE(std::abs(props[0] - 10.) < tol);
      EXPECT_TRUE(std::abs(props[1] - 2.) < tol);
      EXPECT_TRUE(hardening == Details::Hardening::LINEAR_ISOTROPIC);
      EXPECT_TRUE(rate_dep == Details::RateDependence::NONE);
    }

    {
      // Power law
      p0.set<std::string>("hardening", "power law");
      auto params = ParameterList("model");
      params.set("plastic", p0);
      std::vector<scalar_type> props;
      Details::Hardening hardening;
      Details::RateDependence rate_dep;
      Details::read_and_validate_plastic_params(params, props, hardening, rate_dep);
      EXPECT_TRUE(props.size() == 8);
      EXPECT_TRUE(std::abs(props[0] - 10.) < tol);
      EXPECT_TRUE(std::abs(props[1] - 2.) < tol);
      EXPECT_TRUE(std::abs(props[2] - .1) < tol);
      EXPECT_TRUE(hardening == Details::Hardening::POWER_LAW);
      EXPECT_TRUE(rate_dep == Details::RateDependence::NONE);
    }

    {
      // Johnson Cook
      p0.set<std::string>("hardening", "johnson cook");
      auto params = ParameterList("model");
      params.set("plastic", p0);
      std::vector<scalar_type> props;
      Details::Hardening hardening;
      Details::RateDependence rate_dep;
      Details::read_and_validate_plastic_params(params, props, hardening, rate_dep);
      EXPECT_TRUE(props.size() == 8);
      EXPECT_TRUE(std::abs(props[0] - 10.) < tol);
      EXPECT_TRUE(std::abs(props[1] - 2.) < tol);
      EXPECT_TRUE(std::abs(props[2] - .1) < tol);
      EXPECT_TRUE(std::abs(props[3] - 400.) < tol);
      EXPECT_TRUE(std::abs(props[4] - 500.) < tol);
      EXPECT_TRUE(std::abs(props[5] - .2) < tol);
      EXPECT_TRUE(hardening == Details::Hardening::JOHNSON_COOK);
      EXPECT_TRUE(rate_dep == Details::RateDependence::NONE);
    }

    {
      // Johnson Cook, with rate
      p0.set<std::string>("hardening", "johnson cook");
      p0.set("rate dependent", p1);
      auto params = ParameterList("model");
      params.set("plastic", p0);
      std::vector<scalar_type> props;
      Details::Hardening hardening;
      Details::RateDependence rate_dep;
      Details::read_and_validate_plastic_params(params, props, hardening, rate_dep);
      EXPECT_TRUE(props.size() == 8);
      EXPECT_TRUE(std::abs(props[0] - 10.) < tol);
      EXPECT_TRUE(std::abs(props[1] - 2.) < tol);
      EXPECT_TRUE(std::abs(props[2] - .1) < tol);
      EXPECT_TRUE(std::abs(props[3] - 400.) < tol);
      EXPECT_TRUE(std::abs(props[4] - 500.) < tol);
      EXPECT_TRUE(std::abs(props[5] - .2) < tol);
      EXPECT_TRUE(hardening == Details::Hardening::JOHNSON_COOK);
      EXPECT_TRUE(rate_dep == Details::RateDependence::JOHNSON_COOK);
    }

    {
      // Zerilli Armstrong, with rate
      auto p_za = ParameterList("plastic");
      p_za.set<std::string>("hardening", "zerilli armstrong");
      p_za.set<scalar_type>("A", 1.);
      p_za.set<scalar_type>("B", 2.);
      p_za.set<scalar_type>("N", 3.);
      p_za.set<scalar_type>("C1", 4.);
      p_za.set<scalar_type>("C2", 5.);
      p_za.set<scalar_type>("C3", 6.);

      auto p_za_r = ParameterList("rate dependent");
      p_za_r.set<std::string>("type", "zerilli armstrong");
      p_za_r.set<double>("C4", 7.);
      p_za.set("rate dependent", p_za_r);

      auto params = ParameterList("model");
      params.set("plastic", p_za);

      std::vector<scalar_type> props;
      Details::Hardening hardening;
      Details::RateDependence rate_dep;
      Details::read_and_validate_plastic_params(params, props, hardening, rate_dep);
      EXPECT_TRUE(props.size() == 8);
      EXPECT_TRUE(std::abs(props[0] - 1.) < tol);
      EXPECT_TRUE(std::abs(props[1] - 2.) < tol);
      EXPECT_TRUE(std::abs(props[2] - 3.) < tol);
      EXPECT_TRUE(std::abs(props[3] - 4.) < tol);
      EXPECT_TRUE(std::abs(props[4] - 5.) < tol);
      EXPECT_TRUE(std::abs(props[5] - 6.) < tol);
      EXPECT_TRUE(std::abs(props[6] - 7.) < tol);
      EXPECT_TRUE(hardening == Details::Hardening::ZERILLI_ARMSTRONG);
      EXPECT_TRUE(rate_dep == Details::RateDependence::ZERILLI_ARMSTRONG);
    }
  }
}


TEST(HyperEPMaterialModel, NeoHookeanHyperElastic)
{
  scalar_type rho = 1.;
  scalar_type temp = 298.;
  scalar_type dtime = 1.;

  tensor_type T;
  tensor_type F = Omega_h::identity_matrix<3, 3>();
  tensor_type Fp = Omega_h::identity_matrix<3, 3>();

  scalar_type E = 10.;
  scalar_type Nu = .1;
  scalar_type A = std::numeric_limits<scalar_type>::max();
  std::vector<scalar_type> props {E, Nu, A};

  scalar_type C10 = E / (4. * (1. + Nu));
  scalar_type D1 = 6. * (1. - 2. * Nu) / E;

  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::NONE;
  auto rate_dep = Details::RateDependence::NONE;

  // Uniaxial strain
  scalar_type f1 = 1.1;
  F(0,0) = f1;
  scalar_type c = 0.;
  scalar_type ep = 0.;
  scalar_type epdot = 0.;
  Details::eval(elastic, hardening, rate_dep, props, rho,
                F, dtime, temp, T, c, Fp, ep, epdot);

  scalar_type fac = 1.66666666666667;  // 10/6
  scalar_type sxx = (2.0/3.0)*std::pow(f1,-fac)*(-2.*C10*D1*(-f1*f1+1)+3*std::pow(f1,fac)*(f1-1))/D1;
  scalar_type syy = (2.0/3.0)*std::pow(f1,-fac)*(C10*D1*(-f1*f1+1)+3*std::pow(f1,fac)*(f1-1))/D1;

  EXPECT_TRUE(Omega_h::are_close(T(0,0), sxx, 5e-7));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), syy, 5e-7));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), T(2,2)));
  EXPECT_TRUE(Omega_h::are_close(T(0,1), 0.0));
  EXPECT_TRUE(Omega_h::are_close(ep, 0.0));
}


TEST(HyperEPMaterialModel, LinearElastic)
{
  scalar_type rho = 1.;
  scalar_type temp = 298.;
  scalar_type dtime = 1.;

  tensor_type T;
  tensor_type F = Omega_h::identity_matrix<3, 3>();
  tensor_type Fp = Omega_h::identity_matrix<3, 3>();

  scalar_type E = 10.;
  scalar_type Nu = .1;
  scalar_type A = std::numeric_limits<scalar_type>::max();
  std::vector<scalar_type> props {E, Nu, A};

  auto elastic = Details::Elastic::LINEAR_ELASTIC;
  auto hardening = Details::Hardening::NONE;
  auto rate_dep = Details::RateDependence::NONE;

  // Uniaxial strain
  scalar_type eps = .1;
  F(0,0) = 1.0 + eps;
  scalar_type c = 0.;
  scalar_type ep = 0.;
  scalar_type epdot = 0.;
  Details::eval(elastic, hardening, rate_dep, props, rho,
                F, dtime, temp, T, c, Fp, ep, epdot);

  scalar_type K = E / 3. / (1. - 2. * Nu);
  scalar_type G = E / 2. / (1. + Nu);
  scalar_type sxx = K * eps + 4.0 / 3.0 * G * eps;
  scalar_type syy = K * eps - 2.0 / 3.0 * G * eps;
  EXPECT_TRUE(Omega_h::are_close(T(0,0), sxx));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), syy));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), T(2,2)));
  EXPECT_TRUE(Omega_h::are_close(T(0,1), 0.0));
  EXPECT_TRUE(Omega_h::are_close(ep, 0.0));
}


TEST(HyperEPMaterialModel, SimpleJ2)
{
  scalar_type rho = 1.;
  scalar_type temp = 298.;
  scalar_type dtime = 1.;

  tensor_type T;
  tensor_type F = Omega_h::identity_matrix<3, 3>();
  tensor_type Fp = Omega_h::identity_matrix<3, 3>();

  scalar_type E = 10e6;
  scalar_type Nu = 0.1;
  scalar_type A = 40e3;
  std::vector<scalar_type> props {E, Nu, A};

  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::NONE;
  auto rate_dep = Details::RateDependence::NONE;

  scalar_type c = 0.;
  scalar_type ep = 0.;
  scalar_type epdot = 0.;
  // Uniaxial strain
  F(0,0) = 1.004;
  auto err_c =
    Details::eval(elastic, hardening, rate_dep, props, rho,
                  F, dtime, temp, T, c, Fp, ep, epdot);
  EXPECT_TRUE(Omega_h::are_close(ep, 0.));

  c = 0.;
  F(0,0) = 1.005;
  err_c = Details::eval(elastic, hardening, rate_dep, props, rho,
                        F, dtime, temp, T, c, Fp, ep, epdot);
  EXPECT_TRUE(Omega_h::are_close(T(0,0), 47500.));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), 7500.));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), T(2,2)));
  EXPECT_TRUE(Omega_h::are_close(ep, 0.00076134126264676861, 1e-6));

}


TEST(HyperEPMaterialModel, NonHardeningRadialReturn)
{
  scalar_type temp = 298.;
  scalar_type dtime = 1.;

  tensor_type T;
  tensor_type F = Omega_h::identity_matrix<3, 3>();
  tensor_type Fp = Omega_h::identity_matrix<3, 3>();

  scalar_type E = 10e6;
  scalar_type Nu = 0.1;
  scalar_type A = 40e3;
  std::vector<scalar_type> props {E, Nu, A};

  auto hardening = Details::Hardening::NONE;
  auto rate_dep = Details::RateDependence::NONE;

  scalar_type ep = 0.;
  scalar_type epdot = 0.;

  Details::ErrorCode err;
  Details::StateFlag flag;

  // Uniaxial stress, below yield
  tensor_type Te = Omega_h::zero_matrix<3, 3>();
  scalar_type fac = .9;
  Te(0,0) = fac * A;
  flag = Details::StateFlag::TRIAL;
  err = Details::radial_return(hardening, rate_dep, props, Te, F,
                               temp, dtime, T, Fp, ep, epdot, flag);

  EXPECT_TRUE(Omega_h::are_close(T(0,0), Te(0,0)));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), Te(1,1)));
  EXPECT_TRUE(Omega_h::are_close(T(2,2), Te(2,2)));
  EXPECT_TRUE(Omega_h::are_close(T(0,1), 0.));
  EXPECT_TRUE(Omega_h::are_close(T(0,2), 0.));
  EXPECT_TRUE(Omega_h::are_close(T(1,2), 0.));

  // Uniaxial stress, above yield
  fac = 1.1;
  Te(0,0) = fac * A;
  flag = Details::StateFlag::TRIAL;
  err = Details::radial_return(hardening, rate_dep, props, Te, F,
                               temp, dtime, T, Fp, ep, epdot, flag);

  scalar_type Txx = 2.*std::pow(A,2)*fac/(3.*A*fac) + A*fac/3.;
  scalar_type Tyy =   -std::pow(A,2)*fac/(3.*A*fac) + A*fac/3.;
  EXPECT_TRUE(Omega_h::are_close(T(0,0), Txx));
  EXPECT_TRUE(Omega_h::are_close(T(1,1), Tyy));
  EXPECT_TRUE(Omega_h::are_close(T(0,1), 0.));
  EXPECT_TRUE(Omega_h::are_close(T(0,2), 0.));
  EXPECT_TRUE(Omega_h::are_close(T(1,2), 0.));
}


TEST(HyperEPMaterialModel, ElasticPerfectlyPlastic)
{
  scalar_type eps = 0.01;
  auto rho = hyper_ep_utils::copper_density();
  auto props = hyper_ep_utils::copper_johnson_cook_props();
  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::NONE;
  auto rate_dep = Details::RateDependence::NONE;
  hyper_ep_utils::eval_prescribed_motions(eps, elastic, hardening, rate_dep, props, rho);
}


TEST(HyperEPMaterialModel, LinearIsotropicHardening)
{
  scalar_type eps = 0.01;
  auto rho = hyper_ep_utils::copper_density();
  auto props = hyper_ep_utils::copper_johnson_cook_props();
  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::LINEAR_ISOTROPIC;
  auto rate_dep = Details::RateDependence::NONE;
  hyper_ep_utils::eval_prescribed_motions(eps, elastic, hardening, rate_dep, props, rho);
}


TEST(HyperEPMaterialModel, PowerLawHardening)
{
  scalar_type eps = 0.01;
  auto rho = hyper_ep_utils::copper_density();
  auto props = hyper_ep_utils::copper_johnson_cook_props();
  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::POWER_LAW;
  auto rate_dep = Details::RateDependence::NONE;
  hyper_ep_utils::eval_prescribed_motions(eps, elastic, hardening, rate_dep, props, rho);
}


TEST(HyperEPMaterialModel, JohnsonCookHardening)
{
  scalar_type eps = 0.01;
  auto rho = hyper_ep_utils::copper_density();
  auto props = hyper_ep_utils::copper_johnson_cook_props();
  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::JOHNSON_COOK;
  auto rate_dep = Details::RateDependence::NONE;
  hyper_ep_utils::eval_prescribed_motions(eps, elastic, hardening, rate_dep, props, rho);
}


TEST(HyperEPMaterialModel, JohnsonCookRateDependent)
{
  scalar_type eps = 0.01;
  auto rho = hyper_ep_utils::copper_density();
  auto props = hyper_ep_utils::copper_johnson_cook_props();
  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::JOHNSON_COOK;
  auto rate_dep = Details::RateDependence::JOHNSON_COOK;
  hyper_ep_utils::eval_prescribed_motions(eps, elastic, hardening, rate_dep, props, rho);
}

TEST(HyperEPMaterialModel, ZerilliArmstrongHardening)
{
  scalar_type eps = 0.01;
  auto rho = hyper_ep_utils::copper_density();
  auto props = hyper_ep_utils::copper_zerilli_armstrong_props();
  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::ZERILLI_ARMSTRONG;
  auto rate_dep = Details::RateDependence::NONE;
  hyper_ep_utils::eval_prescribed_motions(eps, elastic, hardening, rate_dep, props, rho);
}


TEST(HyperEPMaterialModel, ZerilliArmstrongRateDependent)
{
  scalar_type eps = 0.01;
  auto rho = hyper_ep_utils::copper_density();
  auto props = hyper_ep_utils::copper_zerilli_armstrong_props();
  auto elastic = Details::Elastic::NEO_HOOKEAN;
  auto hardening = Details::Hardening::ZERILLI_ARMSTRONG;
  auto rate_dep = Details::RateDependence::ZERILLI_ARMSTRONG;
  hyper_ep_utils::eval_prescribed_motions(eps, elastic, hardening, rate_dep, props, rho);
}

} // namespace

