#ifndef LGR_FACTORIES_HPP
#define LGR_FACTORIES_HPP

#include <functional>
#include <Omega_h_teuchos.hpp>
#include <lgr_element_types.hpp>

namespace lgr {

struct Simulation;
struct ModelBase;
struct Scalar;
struct Response;

template <class T>
using PtrOf = T*;
template <class T>
using FactoryOf = std::function<PtrOf<T>(Simulation&, std::string const&, Teuchos::ParameterList&)>;
template <class T>
using FactoriesOf = std::map<std::string, FactoryOf<T>>;
template <class T>
using VectorOf = std::vector<std::unique_ptr<T>>;

using ModelPtr = PtrOf<ModelBase>;
using ModelFactory = FactoryOf<ModelBase>;
using ModelFactories = FactoriesOf<ModelBase>;

using ScalarPtr = PtrOf<Scalar>;
using ScalarFactory = FactoryOf<Scalar>;
using ScalarFactories = FactoriesOf<Scalar>;

using ResponsePtr = PtrOf<Response>;
using ResponseFactory = FactoryOf<Response>;
using ResponseFactories = FactoriesOf<Response>;

struct Factories {
  Factories() = default;
  Factories(std::string const& element_type);
  ModelFactories material_model_factories;
  ModelFactories modifier_factories;
  ModelFactories field_update_factories;
  ScalarFactories scalar_factories;
  ResponseFactories response_factories;
  bool empty();
};

template <class T>
void setup(FactoriesOf<T> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<T>&, std::string const&);
extern template void
setup(FactoriesOf<ModelBase> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<ModelBase>&, std::string const&);
extern template void
setup(FactoriesOf<Scalar> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<Scalar>&, std::string const&);
extern template void
setup(FactoriesOf<Response> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<Response>&, std::string const&);

}

#endif
