//
// Created by iensen on 8/19/16.
//

#include <groundplog/solver_strategies.h>

GroundPlog::ResultHandler::~ResultHandler() {
    throw "not implemented yet";
}

bool GroundPlog::ResultHandler::onUnsat(const GroundPlog::Solver &) {
	throw "not implemented yet";
}

bool GroundPlog::ResultHandler::onResult(const GroundPlog::Solver &, double result) {
	throw "not implemented yet";
}

bool GroundPlog::ResultHandler::onNonDCO(const GroundPlog::Solver &) {
	throw "not implemented yet";
}

GroundPlog::Configuration *GroundPlog::Configuration::config(const char *n) {
	throw "not implemented yet";
}


GroundPlog::BasicSatConfig::BasicSatConfig() {
	opts_ = SolverParams();
	heu_ = 0;
}
void GroundPlog::BasicSatConfig::reset() {
    throw "not implemented yet";
}

void GroundPlog::BasicSatConfig::resize(uint32 numSolver, uint32 numSearch) {
    throw "not implemented yet";
}

void GroundPlog::BasicSatConfig::setHeuristicCreator(GroundPlog::BasicSatConfig::HeuristicCreator hc) {
    throw "not implemented yet";
}

void GroundPlog::BasicSatConfig::prepare(GroundPlog::SharedContext &) {
    throw "not implemented yet";
}


GroundPlog::DecisionHeuristic *GroundPlog::BasicSatConfig::heuristic(uint32 i) const {
    throw "not implemented yet";
}

GroundPlog::Configuration::SolverOpts &GroundPlog::BasicSatConfig::addSolver(uint32 i) {
    throw "not implemented yet";
}

GroundPlog::Configuration::~Configuration() {
	throw "not implemented yet";
}
