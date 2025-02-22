#include <lgr_factories.hpp>
#include <lgr_element_types.hpp>

#include <lgr_models.hpp>
#include <lgr_scalars.hpp>
#include <lgr_responses.hpp>

namespace lgr {

Factories::Factories(std::string const& element_type) {
  response_factories = get_builtin_response_factories();
  scalar_factories = get_builtin_scalar_factories();
#define LGR_EXPL_INST(Elem) \
  if (element_type == Elem::name()) { \
    material_model_factories = get_builtin_material_model_factories<Elem>(); \
    modifier_factories = get_builtin_modifier_factories<Elem>(); \
    field_update_factories = get_builtin_field_update_factories<Elem>(); \
    return; \
  }
LGR_EXPL_INST_ELEMS
#undef LGR_EXPL_INST
  Omega_h_fail("Unknown element type \"%s\"\n", element_type.c_str());
}

bool Factories::empty() {
  return material_model_factories.empty() && scalar_factories.empty() && response_factories.empty();
}

template <class T>
static T* get(FactoriesOf<T> const& factories, std::string const& name,
    Simulation& sim, Teuchos::ParameterList& pl, std::string const& category_name) {
  auto type_name = pl.get<std::string>("type");
  auto factory_it = factories.find(type_name);
  if (factory_it == factories.end()) {
    Omega_h_fail("no %s factory for type \"%s\"\n", category_name.c_str(), type_name.c_str());
  }
  auto& factory = factory_it->second;
  return factory(sim, name, pl);
}

template <class T>
void setup(FactoriesOf<T> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<T>& out, std::string const& category_name) {
  for (auto it = pl.begin(), end = pl.end(); it != end; ++it) {
    auto name = pl.name(it);
    auto& subpl = pl.sublist(name);
    auto ptr = get(factories, name, sim, subpl, category_name);
    std::unique_ptr<T> unique_ptr(ptr);
    out.push_back(std::move(unique_ptr));
  }
}

template void
setup(FactoriesOf<ModelBase> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<ModelBase>&, std::string const&);
template void
setup(FactoriesOf<Scalar> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<Scalar>&, std::string const&);
template void
setup(FactoriesOf<Response> const& factories, Simulation& sim, Teuchos::ParameterList& pl, VectorOf<Response>&, std::string const&);

}
