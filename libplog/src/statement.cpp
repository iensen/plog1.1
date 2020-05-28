//
// Created by iensen on 10/14/16.
//
#include<plog/input/statement.h>
#include<gringo/input/statement.hh>
#include <sstream>
#include<plog/input/utils.h>
#include <plog/ploggrammar.tab.hh>

using Gringo::make_locatable;
using Gringo::gringo_make_unique;
using Gringo::FunctionTerm;
using Gringo::VarTermSet;
using Gringo::ValTerm;
using Gringo::VarTerm;
using Gringo::Symbol;
using Gringo::Location;
using Gringo::make_locatable;
using Gringo::Input::PredicateLiteral;
using GStatement = Gringo::Input::Statement;
using Gringo::Input::SimpleHeadLiteral;
using Gringo::Input::SimpleBodyLiteral;
using USimpleHeadLit = std::unique_ptr<SimpleHeadLiteral>;

// TODO: move all "locations" to a static variable
// TODO: add consts to function signatures (expecially those which are unique pointers references)
int Statement::rule_id = 0;



Statement::Statement(Plog::ULit &&head, Plog::ULitVec &&body):head_(std::move(head)),body_(std::move(body)),type_(StatementType::RULE){
}

Statement::Statement(Plog::ULit &&head, Plog::ULitVec &&body, UProb &&prob):head_(std::move(head)),body_(std::move(body)),probability_(std::move(prob)),type_(StatementType::PR_ATOM){

}

Statement::Statement(Plog::ULit &&query):head_(std::move(query)),type_(StatementType::QUERY) {
}

void Statement::print(std::ostream &out) const {
    throw "not implemented yet";
}

void Statement::check(Logger &log) const {
    throw "not implemented yet";
}

void Statement::replace(Defines &dx) {
    throw "not implemented yet";
}

void Statement::toGround(ToGroundArg &x, UStmVec &stms) const {
    throw "not implemented yet";
}

Statement::~Statement() {
}

// lit here must of the the form a rel b,
// ehre rel in {=,!=} and a is an attribute term
// for a(t) = y, return <a(t,y), true>
// for a(t) != y, return <a(t,y), false>
// for a term of the form term rel term, where term is not con
// structed from an attribute term, return {term rel term, true}
// to do: get rid of this and construct clingo terms directly
std::pair<Gringo::UTerm, bool>  Statement::term(Plog::ULit & lit) {
        if (FunctionTerm *fterm = dynamic_cast<FunctionTerm *>(lit->lt.get())) {
            String name = fterm->name;
            UTermVec &args = fterm->args;
            std::unique_ptr<Term> ut(lit->rt->clone());
            args.push_back(std::move(ut));
            Gringo::UTerm term = make_locatable<FunctionTerm>(DefaultLocation(), name, clone(args));
            bool pos = lit->rel == Relation::EQ;
            return {std::move(term), pos};
        } else {
            ValTerm* vterm = dynamic_cast<ValTerm *>(lit->lt.get());
            String name = vterm->value.name();
            UTermVec  args;
            std::unique_ptr<Term> ut(lit->rt->clone());
            args.push_back(std::move(ut));
            Gringo::UTerm term = make_locatable<FunctionTerm>(DefaultLocation(), name, clone(args));
            bool pos = lit->rel == Relation::EQ;
            return {std::move(term), pos};
        }
}



StatementType Statement::getType() {
    return type_;
}



std::vector<Clingo::AST::Statement> Statement::toGringoAST(const UAttDeclVec & attdecls, const USortDefVec &sortDefVec,
        AlgorithmKind algo) {
    switch(type_) {
        case StatementType::PR_ATOM: return prAtomToGringoAST(attdecls, sortDefVec);
        case StatementType::QUERY:   return queryToGringoAST(attdecls, algo);
        default: return ruleToGringoAST(attdecls, sortDefVec, algo);
    }
}



// TODO: rename sortDefVec
std::vector<Clingo::AST::Statement> Statement::prAtomToGringoAST(const UAttDeclVec & attdecls, const USortDefVec &sortDefVec) {
    // construct head:
    Clingo::Location loc("<test>", "<test>", 1, 1, 1, 1);
    std::vector<Clingo::AST::Term> args;
    args.push_back(termToClingoTerm(head_->lt));
    args.push_back(termToClingoTerm(head_->rt));
    std::stringstream stream;
    stream << probability_->getNum();
    args.push_back(Clingo::AST::Term{loc,Clingo::String(stream.str().c_str())});
    stream.str("");
    stream << probability_->getDenum();
    args.push_back(Clingo::AST::Term{loc,Clingo::String(stream.str().c_str())});
    Clingo::AST::Function f_ = {"__pr", args};
    Clingo::AST::Term f_t{loc, f_};
    Clingo::AST::Literal f_l{loc, Clingo::AST::Sign::None, f_t};
    Clingo::AST::Rule f_r{{loc, f_l}, gringobody(attdecls, sortDefVec)};
    return {{loc, f_r},make_external_atom_rule(attdecls, sortDefVec)};
}
// for dco algo, and query ?p(a) we construct __query(p(a), true) -- full representation of the query
// for naive algo, we construct __query :- p(a), since we just need to know whether or not the query was true
std::vector<Clingo::AST::Statement> Statement::queryToGringoAST(const UAttDeclVec & attdecls, AlgorithmKind algo) {
    Clingo::Location loc("<test>", "<test>", 1, 1, 1, 1);
    if(algo == AlgorithmKind::for_dco) {
        std::vector<Clingo::AST::Term> args;
        auto queryargs = term(head_);
        args.push_back(termToClingoTerm(queryargs.first));
        args.push_back(queryargs.second ? Clingo::AST::Term{loc, Clingo::Id("true")} : Clingo::AST::Term{loc,
                                                                                                         Clingo::Id(
                                                                                                                 "false")});
        Clingo::AST::Function f_ = {"__query", args};
        Clingo::AST::Term f_t{loc, f_};
        Clingo::AST::Literal f_l{loc, Clingo::AST::Sign::None, f_t};
        Clingo::AST::Rule f_r{{loc, f_l}, std::vector<Clingo::AST::BodyLiteral>()};
        return {{loc, f_r}};
    } else {
        Clingo::AST::Function f_ = {"__query", {}};
        Clingo::AST::Term f_t{loc, f_};
        Clingo::AST::Literal f_l{loc, Clingo::AST::Sign::None, f_t};
        std::pair<Gringo::UTerm, bool> termb = term(head_); //3
        if(!termb.second) {
            throw "not implemented";
        }
        Clingo::AST::Term f_bt = termToClingoTerm(termb.first);
        Clingo::AST::Literal alit{defaultLoc, Clingo::AST::Sign::None, f_bt};
        auto bodyLit =  Clingo::AST::BodyLiteral{defaultLoc, Clingo::AST::Sign::None, alit};
        Clingo::AST::Rule f_r{{loc, f_l}, std::vector<Clingo::AST::BodyLiteral>{bodyLit}};
        return {{loc, f_r}};
    }
}

std::vector<Clingo::AST::Statement> Statement::ruleToGringoAST(const UAttDeclVec & attdecls, const USortDefVec &sortDefVec, AlgorithmKind algo) {
    std::vector<Clingo::AST::Statement> result;
    // generate a rule of the form head :- body, sort_atoms, ex_atom
    auto    fterm = term(head_);
    // f_ is either random(a,p), or a = y
    Clingo::AST::Term f_ = termToClingoTerm(fterm.first);
    Clingo::AST::Literal f_l{defaultLoc, Clingo::AST::Sign::None, f_};
    Clingo::AST::Rule f_r{{defaultLoc, f_l}, gringobody(attdecls, sortDefVec)};
    //std::cout <<"f_r: "<< f_r << std::endl;
    result.emplace_back(Clingo::AST::Statement{defaultLoc, f_r});
    // generate a rule of the form #external ex_atom:sort_atoms
    Clingo::AST::Statement ext = make_external_atom_rule(attdecls, sortDefVec);
    result.push_back(ext);
    ++rule_id;

    // if the rule is of the form a(t) = y :- B, where a!=random, a!= obs and a!= do,
    // 1. add the rule a(t,y) :- __ext(a(t,y)) to the program
    // 2. add the rule #external __ext(a(t,y)):sorts for the head to the program
    std::string f_str = term_to_string(f_);
    if(f_str.find("obs(")!=0 && f_str.find("random(")!=0 && f_str.find("do(")!=0) {
        std::vector<Clingo::AST::Term> args;
        args.push_back(f_);
        Clingo::AST::BodyLiteral bodyLit = make_body_lit("__ext", args);
        std::vector<Clingo::AST::BodyLiteral> body;
        body.push_back(bodyLit);
        Clingo::AST::Rule f_r{{defaultLoc, f_l}, body};
        result.emplace_back(Clingo::AST::Statement{defaultLoc, f_r}); //1
        Clingo::AST::Term ext_term = make_term("__ext", args);
        std::vector<Clingo::AST::BodyLiteral> bodylits = getSortAtoms(head_,sortDefVec, attdecls);
        Clingo::AST::External ext = Clingo::AST::External{ext_term,bodylits, make_term("false")};
        result.emplace_back(Clingo::AST::Statement{defaultLoc, ext}); //2
    }

    // if the rule is of the form random(a(t),p) :- B, and the algo is for_dco
    // 1. add rules a(t,y):- __ext(a(t,y)) for every y from the range of a(t)
    // 2. add rules #external __ext(a(t,y)):sorts for the head to the program
    // f_str stores random(a(t),p)
    if(f_str.find("random(") == 0 ) {// if the head is a random atom
        // fgterm stores random(
        FunctionTerm* fgterm = (FunctionTerm*) (fterm.first.get()); //

        std::string attrName ="";
        std::vector<Clingo::AST::Term> args;

        if(f_.data.is<Clingo::AST::Function>()) {
            Clingo::AST::Function ct = f_.data.get<Clingo::AST::Function>();
            attrName = ct.name;
            args = ct.arguments;

        } else if(f_.data.is<Clingo::Symbol>()) {
            Clingo::Symbol st = f_.data.get<Clingo::Symbol>();
            attrName = st.string();
        } else {
            throw std::logic_error("random attribute term not found");
        }

        const UTerm & aterm = fgterm->args[0];
        String termName = getAttrName(aterm);
        // find the list of possible values
        const USortExpr  & resSort = getResultSort(termName,attdecls);
        std::vector<Clingo::AST::Term> instances = resSort->generate(sortDefVec);
        if(algo == AlgorithmKind::for_dco) {
            // add the rule add a(t,instance) :- ext(a(t,instance)) for every instance in intances
            for (const Clingo::AST::Term &y: instances) {
                std::vector<Clingo::AST::Term> argsc = getAttrArgs(aterm);
                argsc.push_back(y);
                Clingo::AST::Literal lit = make_lit(termName.c_str(), argsc);
                std::vector<Clingo::AST::Term> exargs;
                exargs.push_back(make_term(termName.c_str(), argsc));
                Clingo::AST::BodyLiteral bodyLit = make_body_lit("__ext", exargs);
                std::vector<Clingo::AST::BodyLiteral> body;
                body.push_back(bodyLit);
                Clingo::AST::Rule f_r{{defaultLoc, lit}, body};
                // add a(t,y) :- ext(a(t,y))
                result.emplace_back(Clingo::AST::Statement{defaultLoc, f_r}); //1
                Clingo::AST::Term ext_term = make_term("__ext", exargs);
                std::vector<Clingo::AST::BodyLiteral> bodylits = getSortAtoms(head_, sortDefVec, attdecls);
                //add ext(a(y,y)):sorts
                Clingo::AST::External ext = Clingo::AST::External{ext_term, bodylits, make_term("false")};
                result.emplace_back(Clingo::AST::Statement{defaultLoc, ext}); //2
            }
            // add the atom of the form __range(a,s), where a is the term name and s is the sort
            std::vector<Clingo::AST::Term> rargs;
            rargs.push_back(make_term(termName));
            rargs.push_back(make_term(resSort->toString().c_str()));
            Clingo::AST::Literal lit = make_lit("__range", rargs);
            Clingo::AST::Rule f_r{{defaultLoc, lit},
                                  {}};
            result.push_back(Clingo::AST::Statement{defaultLoc, f_r});
        } else {
            assert(algo == AlgorithmKind::naive);
            // add P-log general axioms.
            // a(t,y1) | ... | a(t,yn) :- random(a,p)
            Clingo::AST::Disjunction d;
            for (const Clingo::AST::Term &y: instances) {
                std::vector<Clingo::AST::Term> argsc = getAttrArgs(aterm);
                argsc.push_back(y);
                auto lit = make_lit(termName.c_str(), argsc);
                auto conditionalLit = Clingo::AST::ConditionalLiteral{lit,{}};
                d.elements.push_back(conditionalLit);
            }
            std::vector<Clingo::AST::BodyLiteral> body;
            body.push_back(Clingo::AST::BodyLiteral{defaultLoc,Clingo::AST::Sign::None, f_l});
            Clingo::AST::Rule f_r{{defaultLoc, d}, body};
            result.push_back(Clingo::AST::Statement{defaultLoc, f_r});
        }
    }

    //std::cout << "result for the rule" << std::endl;
    //for(const auto & c : result) {
    //    std::cout << c <<std::endl;
    //}

    return result;
}

// do not pass e-literal here!
// !! THIS NEEDS A REVISION
std::vector<Clingo::AST::BodyLiteral> Statement::getSortAtoms(const Plog::ULit & lit, const USortDefVec &sortDefVec,const UAttDeclVec & attdecls) {
    // add assert that it is not an e-literal
    std::vector<Clingo::AST::BodyLiteral> result;
    String attrName = lit->getAttrName();
    if(attrName == "") {
        return {};
    }

    FunctionTerm * fterm = dynamic_cast<FunctionTerm*>(lit->lt.get());
    bool isRandom = attrName == "random";

    // TODO: Create doTerm, obsTerm, etc with corresponding methods.
    if(attrName == "random" || attrName == "pr" || attrName == "obs" || attrName == "do") {

        int att_idx = 0;
        bool with_agent = false;
        if(attrName == "obs" && fterm->args.size() == 5
                || attrName == "do" && fterm->args.size()==4) {
            ++att_idx;
            with_agent = true;
        }

        if(!isRandom) {
            if(with_agent) {
                const UTerm & agent = fterm->args[0];
                std::vector<Clingo::AST::Term> args2;
                args2.push_back(termToClingoTerm(agent));
                Clingo::AST::BodyLiteral bodylit = make_body_lit("_agent", args2);
                result.push_back(bodylit);

            }
            const UTerm & attribute_term = fterm->args[att_idx];
            const UTerm & value = fterm->args[att_idx + 1];
            std::vector<Clingo::AST::Term> args2;
            args2.push_back(termToClingoTerm(value));
            String attrName3="";
            if(dynamic_cast<FunctionTerm*>(attribute_term.get())) {
                attrName3 = dynamic_cast<FunctionTerm *>(attribute_term.get())->name;
            } else{
                attrName3 = dynamic_cast<ValTerm *>(attribute_term.get())->value.name();
            }
            std::vector<String> argSorts2 = findArgSorts(attrName3, attdecls);
            String sortName = argSorts2[argSorts2.size() - 1];
            Clingo::AST::BodyLiteral bodylit = make_body_lit(concat('_', sortName), args2);
            result.push_back(bodylit);
        }
        fterm = dynamic_cast<FunctionTerm*>(fterm->args[0].get());
        if(fterm)
            attrName = fterm->name;

        // take the term from the first argument!

    }
    std::vector<String> argSorts = findArgSorts(attrName, attdecls);

    if(fterm) {
        const UTermVec  & targs = fterm->args;
        for(int i=0; i< argSorts.size()-1;i++) {
            // for now assume attribute declaration may only contain sort names.
            String sortName = argSorts[i];
            std::unique_ptr<Term> ut(targs[i]->clone());
            std::vector<Clingo::AST::Term> args;
            args.push_back(termToClingoTerm(ut));
            Clingo::AST::BodyLiteral bodylit = make_body_lit(concat('_',sortName),args);
            result.push_back(bodylit);
        }
    }

    // add the sort for the value:
    // note that there is no value for random, do and obs  atoms,
    if(!isRandom && attrName != "obs" && attrName != "do") {
        std::vector<Clingo::AST::Term> args;
        std::unique_ptr<Term> ut(lit->rt->clone());
        args.push_back(termToClingoTerm(ut));
        String sortName = argSorts[argSorts.size() - 1];
        Clingo::AST::BodyLiteral bodylit = make_body_lit(concat('_', sortName), args);
        result.push_back(bodylit);
    }

    return result;
}

std::vector<Clingo::AST::BodyLiteral> Statement::gringobody(const UAttDeclVec &attdecls, const USortDefVec &sortDefVec) {
    std::vector<Clingo::AST::BodyLiteral> resvec;

    // add literals obtain from the given body:
    for (auto& lit: body_) {
        resvec.push_back(gringobodyexlit(lit,attdecls));
    }
    // add sorts here
    std::vector<Clingo::AST::BodyLiteral> sortAtoms = getSortAtoms(sortDefVec, attdecls);
    resvec.insert(resvec.end(), sortAtoms.begin(), sortAtoms.end());
    // add external __ext(id, X1,...,Xn) to the body of the rule
    Clingo::AST::Term extterm = make_external_term();
    Clingo::AST::Literal extlit ={defaultLoc, Clingo::AST::Sign::None, extterm};
    resvec.emplace_back(Clingo::AST::BodyLiteral{defaultLoc,Clingo::AST::Sign::None, extlit});
    return resvec;
}


Clingo::AST::BodyLiteral Statement::gringobodyexlit(Plog::ULit &lit, const UAttDeclVec &attdecls) {
    Plog::ELiteral *elit = dynamic_cast<Plog::ELiteral *>(lit.get());

    if (!elit->isRelational(attdecls)) {
        std::pair<Gringo::UTerm, bool> termb = term(elit->lit); //3
        Clingo::AST::Term f_t = termToClingoTerm(termb.first);
        Clingo::AST::Sign sign = Clingo::AST::Sign::None;
        if (!termb.second) {
           // std::stringstream ss;
            //ss <<"-";
            //termb.first->print(ss);
            //std::cout << ss.str()<< std::endl;
            //auto funct = dynamic_cast<Gringo::FunctionTerm*>(termb.first.get());

            //if(!funct) {
                throw "not implemented yet";
            //}
           // Symbol cf = Clingo::Function{funct->name,funct->args};

            //)f_t = {defaultLoc, sym};
        }
        Clingo::AST::Literal alit{defaultLoc, Clingo::AST::Sign::None, f_t};

        //elit->neg = ;
        return Clingo::AST::BodyLiteral{defaultLoc, elit->neg ? Clingo::AST::Sign::Negation : Clingo::AST::Sign::None,
                                        alit};
    } else {
        Clingo::AST::Term left_clingo_term = termToClingoTerm(elit->lit->lt);
        Clingo::AST::Term right_clingo_term = termToClingoTerm(elit->lit->rt);
        Clingo::AST::Comparison cmp{getComparisonOpFromRelation(elit->lit->rel), left_clingo_term, right_clingo_term};
        Clingo::AST::Literal alit{defaultLoc, Clingo::AST::Sign::None, cmp};
        return Clingo::AST::BodyLiteral{defaultLoc, Clingo::AST::Sign::None, alit};
    }
}



std::unordered_set<std::string> Statement::getVariables() {
    auto result = getVariables(head_->lt);
    auto hrtvars = getVariables(head_->rt);
    result.insert(hrtvars.begin(), hrtvars.end());
    // add variables from the body
    for(const auto& lit:body_) {
        Plog::ELiteral* elit = dynamic_cast<Plog::ELiteral*>(lit.get());
        auto blvarsl = getVariables(elit->lit->lt);
        auto blvarsr = getVariables(elit->lit->rt);
        result.insert(blvarsl.begin(), blvarsl.end());
        result.insert(blvarsr.begin(), blvarsr.end());
    }
    return result;
}

std::unordered_set<std::string> Statement::getVariables(const UTerm &term) {
    std::unordered_set<std::string> result;
    VarTermSet vars;
    term->collect(vars);
    for(const auto &var : vars) {
        std::string varstr {var.get().name.c_str()};
        result.insert(varstr);
    }
    return result;
}



// todo use references if possible:
Clingo::AST::BodyLiteral Statement::make_body_lit(String name, std::vector<Clingo::AST::Term> args) {
    return {defaultLoc,Clingo::AST::Sign::None, make_lit(name,args)};
}

std::vector<String> Statement::findArgSorts(String attrName,const UAttDeclVec & attdecls) {
    std::vector<String> argSorts;
    // find attribute declaration:
    for(const UAttDecl& dec: attdecls) {
        if( dec->attname == attrName) {
            String ressortName = ((SortNameExpr*)dec->se.get())->name;
            for(int i=0;i< dec->svec.size();i++) {
                String sortName = ((SortNameExpr*)dec->svec.at(i).get())->name;
                argSorts.push_back(sortName);
            }
            argSorts.push_back(ressortName);
        }
    }
    return argSorts;
}

Clingo::AST::Literal Statement::make_lit(String name, std::vector<Clingo::AST::Term> args) {
    return {defaultLoc, Clingo::AST::Sign::None, make_term(name, args)};
}

std::vector<Clingo::AST::BodyLiteral>
Statement::getSortAtoms(const USortDefVec &sortDefVec, const UAttDeclVec &attdecls) {
    std::vector<Clingo::AST::BodyLiteral> resvec = getSortAtoms(head_, sortDefVec, attdecls);
    // add sorts for the body:
    for(const Plog::ULit& blit:body_) {
        if(!blit->isRelational(attdecls)) {
            Plog::ELiteral *elit = dynamic_cast<Plog::ELiteral *>(blit.get());
            std::vector<Clingo::AST::BodyLiteral> litSortAtoms = getSortAtoms(elit->lit, sortDefVec, attdecls);
            resvec.insert(resvec.end(), litSortAtoms.begin(), litSortAtoms.end());
        }
    }
    return resvec;
}

Clingo::AST::Term Statement::make_external_term() {
    std::vector<Clingo::AST::Term> args;
    Clingo::AST::Term idarg = Clingo::AST::Term{defaultLoc,Clingo::Id(int_to_str(rule_id).c_str())};
    args.push_back(idarg);
    // insert all the variables
    auto varstrs = getVariables();// this will get destroyed k chertyam by the end of the function call
    for(const std::string &var: varstrs) {
        char * buf = new char [var.length()+1];
        strcpy(buf,var.c_str());
        Clingo::AST::Term vararg = Clingo::AST::Term{defaultLoc,Clingo::AST::Variable{buf}};
        args.push_back(vararg);
    }
    Clingo::AST::Function f_ = {"__ext", args};
    return {defaultLoc, f_};
}

Clingo::AST::Statement Statement::make_external_atom_rule(const UAttDeclVec & attdecls, const USortDefVec &sortDefVec) {
    Clingo::AST::Term exheadterm = make_external_term();
    std::vector<Clingo::AST::BodyLiteral> sortAtoms = getSortAtoms(sortDefVec, attdecls);
    Clingo::AST::External extr =Clingo::AST::External{exheadterm,sortAtoms,make_term("false")};
    return {defaultLoc, extr};
}


Clingo::AST::Term Statement::make_term(String name, std::vector<Clingo::AST::Term> args) {
    Clingo::AST::Function f_ = {name.c_str(), args};
    return {defaultLoc, f_};
}

Clingo::AST::Term Statement::make_term(String name) {
    Clingo::Symbol s = Clingo::Id(name.c_str());
    return {defaultLoc, s};
}

Clingo::AST::ComparisonOperator Statement::getComparisonOpFromRelation(Gringo::Relation rel) {
    switch(rel) {
        case Gringo::Relation::EQ: return Clingo::AST::ComparisonOperator::Equal;
        case Gringo::Relation::GEQ: return Clingo::AST::ComparisonOperator::GreaterEqual;
        case Gringo::Relation::GT: return Clingo::AST::ComparisonOperator::GreaterThan;
        case Gringo::Relation::LEQ: return Clingo::AST::ComparisonOperator::LessEqual;
        case Gringo::Relation::LT: return Clingo::AST::ComparisonOperator::LessThan;
        case Gringo::Relation::NEQ: return Clingo::AST::ComparisonOperator::NotEqual;
    }
}
