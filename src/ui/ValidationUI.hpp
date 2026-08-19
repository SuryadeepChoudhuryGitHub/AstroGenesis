#pragma once

#include "imgui.h"
#include "simulation/ValidationEngine.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "data/repositories/ObjectRepository.hpp"

namespace AstroGenesis {

class ValidationUI {
public:
    ValidationUI();

    void render(bool& showWindow, 
                ValidationEngine& valEngine, 
                ObjectRepository& objRepo,
                PhysicsEngine& physics,
                float winW, float winH);

private:
    void drawTrajectoryPlot(const std::vector<ValidationComparisonPoint>& points, float w, float h);
    void drawErrorCurvePlot(const std::vector<ValidationComparisonPoint>& points, float w, float h);

    int m_selectedBodyIdx = 0;
    float m_durationDays = 365.25f;
    bool m_enableGR = true;
    bool m_hasBenchmarkRun = false;
    bool m_hasGRComparisonRun = false;

    std::vector<ValidationComparisonPoint> m_benchmarkPoints;
    ValidationBenchmarkSummary m_benchmarkSummary;

    std::vector<ValidationComparisonPoint> m_newtonPoints;
    std::vector<ValidationComparisonPoint> m_grPoints;
    ValidationBenchmarkSummary m_newtonSummary;
    ValidationBenchmarkSummary m_grSummary;

    std::string m_errorMessage;
};

} // namespace AstroGenesis
