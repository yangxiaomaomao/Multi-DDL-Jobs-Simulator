/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Denis Fakhriev <fakhriev@iitp.ru>
 */
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/steady-state-random-waypoint-mobility-model.h"
#include "ns3/test.h"

#include <cmath>

using namespace ns3;

/**
 * @ingroup mobility-test
 *
 * @brief Steady State Random Waypoint Test
 */
class SteadyStateRandomWaypointTest : public TestCase
{
  public:
    SteadyStateRandomWaypointTest()
        : TestCase("Check steady-state rwp mobility model velocity and position distributions")
    {
    }

    ~SteadyStateRandomWaypointTest() override
    {
    }

  private:
    std::vector<Ptr<MobilityModel>> mobilityStack; ///< modility model
    double count;                                  ///< count
  private:
    void DoRun() override;
    void DoTeardown() override;
    /// Distribution compare function
    void DistribCompare();
};

void
SteadyStateRandomWaypointTest::DoTeardown()
{
    mobilityStack.clear();
}

void
SteadyStateRandomWaypointTest::DoRun()
{
    SeedManager::SetSeed(123);

    // Total simulation time, seconds
    double totalTime = 1000;

    ObjectFactory mobilityFactory;
    mobilityFactory.SetTypeId("ns3::SteadyStateRandomWaypointMobilityModel");
    mobilityFactory.Set("MinSpeed", DoubleValue(0.01));
    mobilityFactory.Set("MaxSpeed", DoubleValue(20.0));
    mobilityFactory.Set("MinPause", DoubleValue(0.0));
    mobilityFactory.Set("MaxPause", DoubleValue(0.0));
    mobilityFactory.Set("MinX", DoubleValue(0));
    mobilityFactory.Set("MaxX", DoubleValue(1000));
    mobilityFactory.Set("MinY", DoubleValue(0));
    mobilityFactory.Set("MaxY", DoubleValue(600));

    // Populate the vector of mobility models.
    count = 10000;
    for (uint32_t i = 0; i < count; i++)
    {
        // Create a new mobility model.
        Ptr<MobilityModel> model = mobilityFactory.Create()->GetObject<MobilityModel>();
        model->AssignStreams(100 * (i + 1));
        // Add this mobility model to the stack.
        mobilityStack.push_back(model);
        Simulator::Schedule(Seconds(0), &Object::Initialize, model);
    }

    Simulator::Schedule(Seconds(0.001), &SteadyStateRandomWaypointTest::DistribCompare, this);
    Simulator::Schedule(Seconds(totalTime), &SteadyStateRandomWaypointTest::DistribCompare, this);
    Simulator::Stop(Seconds(totalTime));
    Simulator::Run();
    Simulator::Destroy();
}

void
SteadyStateRandomWaypointTest::DistribCompare()
{
    double velocity;
    double sum_x = 0;
    double sum_y = 0;
    double sum_v = 0;
    for (auto i = mobilityStack.begin(); i != mobilityStack.end(); ++i)
    {
        auto model = (*i);
        velocity =
            std::sqrt(std::pow(model->GetVelocity().x, 2) + std::pow(model->GetVelocity().y, 2));
        sum_x += model->GetPosition().x;
        sum_y += model->GetPosition().y;
        sum_v += velocity;
    }
    double mean_x = sum_x / count;
    double mean_y = sum_y / count;
    double mean_v = sum_v / count;

    NS_TEST_EXPECT_MSG_EQ_TOL(mean_x, 500, 25.0, "Got unexpected x-position mean value");
    NS_TEST_EXPECT_MSG_EQ_TOL(mean_y, 300, 15.0, "Got unexpected y-position mean value");
    NS_TEST_EXPECT_MSG_EQ_TOL(mean_v, 2.6, 0.13, "Got unexpected velocity mean value");

    sum_x = 0;
    sum_y = 0;
    sum_v = 0;
    double tmp;
    for (auto i = mobilityStack.begin(); i != mobilityStack.end(); ++i)
    {
        auto model = (*i);
        velocity =
            std::sqrt(std::pow(model->GetVelocity().x, 2) + std::pow(model->GetVelocity().y, 2));
        tmp = model->GetPosition().x - mean_x;
        sum_x += tmp * tmp;
        tmp = model->GetPosition().y - mean_y;
        sum_y += tmp * tmp;
        tmp = velocity - mean_v;
        sum_v += tmp * tmp;
    }
    double dev_x = std::sqrt(sum_x / (count - 1));
    double dev_y = std::sqrt(sum_y / (count - 1));
    double dev_v = std::sqrt(sum_v / (count - 1));

    NS_TEST_EXPECT_MSG_EQ_TOL(dev_x, 230, 10.0, "Got unexpected x-position standard deviation");
    NS_TEST_EXPECT_MSG_EQ_TOL(dev_y, 140, 7.0, "Got unexpected y-position standard deviation");
    NS_TEST_EXPECT_MSG_EQ_TOL(dev_v, 4.4, 0.22, "Got unexpected velocity standard deviation");
}

/**
 * @ingroup mobility-test
 *
 * @brief Steady State Random Waypoint Test Suite
 */
struct SteadyStateRandomWaypointTestSuite : public TestSuite
{
    SteadyStateRandomWaypointTestSuite()
        : TestSuite("steady-state-rwp-mobility-model", Type::UNIT)
    {
        AddTestCase(new SteadyStateRandomWaypointTest, TestCase::Duration::QUICK);
    }
} g_steadyStateRandomWaypointTestSuite; ///< the test suite
