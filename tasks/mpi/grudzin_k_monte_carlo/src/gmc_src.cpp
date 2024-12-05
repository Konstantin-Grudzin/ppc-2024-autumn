#include "mpi/grudzin_k_monte_carlo/include/gmc_include.hpp"

namespace grudzin_k_montecarlo_mpi {
template <const int dimension>
bool MonteCarloMpi<dimension>::pre_processing() {
  internal_order_test();
  return true;
}

template <const int dimension>
bool MonteCarloMpi<dimension>::run() {
  internal_order_test();
  int N = 0;
  int delta;
  int rest;
  if (world.rank() == 0) {
    dim.resize(2 * dimension);
    double* dim_ptr = reinterpret_cast<double*>(taskData->inputs[0]);
    std::copy(dim_ptr, dim_ptr + 2 * dimension, dim.data());
    int* N_ptr = reinterpret_cast<int*>(taskData->inputs[1]);
    std::copy(N_ptr, N_ptr + 1, &N);

    delta = N / world.size();
    rest = N % world.size();
  }
  boost::mpi::broadcast(world, dim, 0);
  boost::mpi::broadcast(world, delta, 0);
  boost::mpi::broadcast(world, rest, 0);

  if (world.rank() < rest) delta++;
  // seed depends on proc rank
  //  make all dot sets in every proc statisticly unique
  std::mt19937 rnd(world.rank() * 1337 + 12);
  std::uniform_real_distribution<> dis(0.0, 1.0);

  double loc_res_ = 0.0;
  for (int i = 0; i < delta; ++i) {
    std::array<double, dimension> x;
    for (int j = 0; j < 2 * dimension; j += 2) {
      x[j / 2] = dim[j] + (dim[j + 1] - dim[j]) * dis(rnd);
    }
    loc_res_ += f(x);
  }
  boost::mpi::reduce(world, loc_res_, result, std::plus<double>(), 0);
  if (world.rank() == 0) {
    double mult = 1.0 / (static_cast<double>(N));
    for (int j = 0; j < 2 * dimension; j += 2) {
      mult *= (dim[j + 1] - dim[j]);
    }

    result *= mult;
  }
  return true;
}

template <const int dimension>
bool MonteCarloMpi<dimension>::post_processing() {
  internal_order_test();
  if (world.rank() == 0) {
    reinterpret_cast<double*>(taskData->outputs[0])[0] = result;
  }
  return true;
}

template <const int dimension>
bool MonteCarloMpi<dimension>::validation() {
  internal_order_test();
  if (world.rank() == 0) {
    if (taskData->outputs_count.size() > 0 && taskData->outputs_count[0] == 1 && taskData->inputs_count.size() == 2 &&
        taskData->inputs_count[0] == dimension) {
      return true;
    }
    return false;
  }
  return true;
}

template <const int dimension>
bool MonteCarloSeq<dimension>::pre_processing() {
  internal_order_test();
  dim.resize(2 * dimension);
  double* dim_ptr = reinterpret_cast<double*>(taskData->inputs[0]);
  std::copy(dim_ptr, dim_ptr + 2 * dimension, dim.data());

  int* N_ptr = reinterpret_cast<int*>(taskData->inputs[1]);
  std::copy(N_ptr, N_ptr + 1, &N);
  return true;
}

template <const int dimension>
bool MonteCarloSeq<dimension>::run() {
  internal_order_test();

  std::mt19937 rnd(12);
  std::uniform_real_distribution<> dis(0.0, 1.0);
  result = 0.0;
  for (int i = 0; i < N; ++i) {
    std::array<double, dimension> x;
    for (int j = 0; j < 2 * dimension; j += 2) {
      x[j / 2] = dim[j] + (dim[j + 1] - dim[j]) * dis(rnd);
    }
    result += f(x);
  }
  double mult = 1.0 / (static_cast<double>(N));
  for (int j = 0; j < 2 * dimension; j += 2) {
    mult *= (dim[j + 1] - dim[j]);
  }
  result *= mult;
  return true;
}

template <const int dimension>
bool MonteCarloSeq<dimension>::post_processing() {
  internal_order_test();
  reinterpret_cast<double*>(taskData->outputs[0])[0] = result;
  return true;
}

template <const int dimension>
bool MonteCarloSeq<dimension>::validation() {
  internal_order_test();
  if (taskData->outputs_count.size() > 0 && taskData->outputs_count[0] == 1 && taskData->inputs_count.size() == 2 &&
      taskData->inputs_count[0] == dimension) {
    return true;
  }
  return false;
}

template class MonteCarloMpi<1>;

template class MonteCarloMpi<2>;

template class MonteCarloMpi<3>;

template class MonteCarloSeq<1>;

template class MonteCarloSeq<2>;

template class MonteCarloSeq<3>;
}  // namespace grudzin_k_montecarlo_mpi