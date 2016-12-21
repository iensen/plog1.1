//
// Created by iensen on 8/19/16.
//

#include <groundplog/groundplog_facade.h>
#include <groundplog/solve_algorithms.h>
#include<clasp/util/timer.h>
#include <clingo.hh>


GroundPlog::GroundPlogFacade::GroundPlogFacade():config_(0) {

}


GroundPlog::GroundPlogFacade::~GroundPlogFacade() {
    throw "not implemented yet";
}

GroundPlog::Program &GroundPlog::GroundPlogFacade::start(GroundPlog::GroundPlogConfig &config) {
    init(config);
    Program* p = new Program();
    p->setOptions(config.prep);
    return *p;
}


GroundPlog::GroundPlogFacade::Result
GroundPlog::GroundPlogFacade::solve(Clingo::Control *clctl) {

    SolveAlgorithm*  algo = config_->solve.createSolveObject();
    return algo->run(static_cast<GroundPlog::Program *>(this->program()), clctl);
}

void GroundPlog::GroundPlogFacade::init(GroundPlog::GroundPlogConfig &cfg) {
    //  ctx.setConfiguration(0, Ownership_t::Retain); // force reload of configuration once done ?? not needed
    config_ = &cfg;
    builder_= new Program();
    builder_->setOptions(cfg.prep);
}




