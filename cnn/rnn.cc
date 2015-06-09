#include "cnn/rnn.h"

#include <string>
#include <cassert>
#include <vector>
#include <iostream>

#include "cnn/nodes.h"

using namespace std;
using namespace cnn::expr;
using namespace cnn;

namespace cnn {

RNNBuilder::~RNNBuilder() {}

SimpleRNNBuilder::SimpleRNNBuilder(unsigned layers,
                       unsigned input_dim,
                       unsigned hidden_dim,
                       Model* model) : layers(layers) {
  unsigned layer_input_dim = input_dim;
  for (unsigned i = 0; i < layers; ++i) {
    Parameters* p_x2h = model->add_parameters({hidden_dim, layer_input_dim});
    Parameters* p_h2h = model->add_parameters({hidden_dim, hidden_dim});
    Parameters* p_hb = model->add_parameters({hidden_dim});
    vector<Parameters*> ps = {p_x2h, p_h2h, p_hb};
    params.push_back(ps);
    layer_input_dim = hidden_dim;
  }
}

void SimpleRNNBuilder::new_graph_impl(ComputationGraph& cg) {
  param_vars.clear();
  for (unsigned i = 0; i < layers; ++i) {
    Parameters* p_x2h = params[i][0];
    Parameters* p_h2h = params[i][1];
    Parameters* p_hb = params[i][2];
    Expression i_x2h =  parameter(cg,p_x2h);
    Expression i_h2h =  parameter(cg,p_h2h);
    Expression i_hb =  parameter(cg,p_hb);

    //VariableIndex i_x2h = cg->add_parameters(p_x2h);
    //VariableIndex i_h2h = cg->add_parameters(p_h2h);
    //VariableIndex i_hb = cg->add_parameters(p_hb);
    //vector<VariableIndex> vars = {i_x2h, i_h2h, i_hb};
    vector<Expression> vars = {i_x2h, i_h2h, i_hb};
    param_vars.push_back(vars);
  }
}

void SimpleRNNBuilder::start_new_sequence_impl(const vector<Expression>& h_0) {
  h.clear();
  h0 = h_0;
  if (h0.size()) { assert(h0.size() == layers); }
}

Expression SimpleRNNBuilder::add_input_impl(Expression& x, ComputationGraph& cg) {
  const unsigned t = h.size();
  h.push_back(vector<Expression>(layers));
  vector<Expression>& ht = h.back();
  Expression in = x;
  for (unsigned i = 0; i < layers; ++i) {
    const vector<Expression>& vars = param_vars[i];
    //Expression i_h3;
    bool have_prev = (t > 0 || h0.size() > 0);
    if (have_prev) {
      Expression i_h_tm1;
      if (t == 0) {
        if (h0.size()) i_h_tm1 = h0[i];  // first time step
      } else {  // tth time step
        i_h_tm1 = h[t-1][i];
      }
      // h3 = hbias + h2h * h_{t-1} + x2h * in
      Expression i_h3 = vars[2] + vars[0] * in + vars[1] * i_h_tm1;
      //i_h3 = cg->add_function<AffineTransform>({vars[2], vars[0], in, vars[1], i_h_tm1});
    } else {
      //i_h3 = cg->add_function<AffineTransform>({vars[2], vars[0], in});
      Expression i_h3 = vars[2] + vars[0] * in ;
    }
    //in = ht[i] = cg->add_function<Tanh>({i_h3});
    in = ht[i] = tanh(i_h3);
  }
  return ht.back();
}

} // namespace cnn
