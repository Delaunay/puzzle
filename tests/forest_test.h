#ifndef PROJECT_TEST_TESTS_ADD_HEADER
#define PROJECT_TEST_TESTS_ADD_HEADER

#include <gtest/gtest.h>

#include <editor/node-editor.h>

TEST(Forest, production_merger_splitter)
{
    Resources::instance().load_configs();

    Forest forest;
    forest.load("merger_splitter_test");
//    auto production = forest.compute_production();

//    for(auto& n: forest.iter_nodes()){
//        debug("{} - {}", n.ID, n.descriptor->name);

//        for(auto& p: n.production){
//            debug("    {} - {} | {} | {}",
//                  p.first, p.second.produced, p.second.received, p.second.consumed);
//        }
//    }

}

#endif

