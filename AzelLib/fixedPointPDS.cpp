#include "PDS.h"

#include <cmath>
#include <limits>

namespace
{
std::int32_t fixedPointFromFloat(double value)
{
    if (std::isnan(value))
    {
        return 0;
    }
    if (value > std::numeric_limits<std::int32_t>::max())
    {
        return std::numeric_limits<std::int32_t>::max();
    }
    if (value < std::numeric_limits<std::int32_t>::min())
    {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(value);
}
}

sVec3_FP FP_Div(sVec3_FP divident, fixedPoint divisor)
{
    sVec3_FP result;
    result[0] = FP_Div(divident[0], divisor);
    result[1] = FP_Div(divident[1], divisor);
    result[2] = FP_Div(divident[2], divisor);

    return result;
}

fixedPoint dot3_FP(const sVec3_FP* r4, const sVec3_FP* r5)
{
    std::uint64_t accumulator = 0;
    accumulator += static_cast<std::uint64_t>(
        static_cast<std::int64_t>((*r4)[0]) * (*r5)[0]
    );
    accumulator += static_cast<std::uint64_t>(
        static_cast<std::int64_t>((*r4)[1]) * (*r5)[1]
    );
    accumulator += static_cast<std::uint64_t>(
        static_cast<std::int64_t>((*r4)[2]) * (*r5)[2]
    );

    return fixedPoint::fromS32(
        fixed_point_detail::truncateShiftedBitsToS32(accumulator, 16)
    );
}

fixedPoint MTH_Product3d_FP(const sVec3_FP& r4, const sVec3_FP& r5)
{
    return dot3_FP(&r4, &r5);
}

sVec3_FP MTH_Mul(const fixedPoint& a, const sVec3_FP& b)
{
    sVec3_FP temp;
    temp[0] = MTH_Mul(a, b[0]);
    temp[1] = MTH_Mul(a, b[1]);
    temp[2] = MTH_Mul(a, b[2]);
    return temp;
}

sVec3_FP MTH_Mul(const sVec3_FP& a, const sVec3_FP& b)
{
    sVec3_FP temp;
    temp[0] = MTH_Mul(a[0], b[0]);
    temp[1] = MTH_Mul(a[1], b[1]);
    temp[2] = MTH_Mul(a[2], b[2]);
    return temp;
}

sVec3_FP MTH_Mul(const sVec3_FP& a, const fixedPoint& b)
{
    sVec3_FP temp;
    temp[0] = MTH_Mul(a[0], b);
    temp[1] = MTH_Mul(a[1], b);
    temp[2] = MTH_Mul(a[2], b);
    return temp;
}

void Imgui_FP(const char* label, fixedPoint* pFP)
{
    float fValue = pFP->toFloat();
    if (ImGui::InputFloat(label, &fValue, 0.01, 0.1))
    {
        const float fixedValue = fValue * 65536.0f;
        pFP->m_value = fixedPointFromFloat(fixedValue);
    }
}

void Imgui_FP_Angle(const char* label, fixedPoint* pFP)
{
    // 1 = sin(1024)
    // 1024 = pi/2
    // conversion is (pi/2)/1024
    float fValueInRad = pFP->toFloat() * ((glm::pi<float>() / 2.f) / 1024.f);
    float fValueInDegree = glm::degrees<float>(fValueInRad);
    if (ImGui::InputFloat(label, &fValueInDegree, 0.01, 0.1))
    {
        fValueInRad = glm::radians<float>(fValueInDegree);

        const float fixedValue =
            (fValueInRad / ((glm::pi<float>() / 2.f) / 1024.f)) * 65536.0f;
        pFP->m_value = fixedPointFromFloat(fixedValue);
    }
}

void Imgui_Vec3FP(sVec3_FP* pVector)
{
    ImGui::PushItemWidth(100);
    Imgui_FP("x", &pVector->m0_X); ImGui::SameLine();
    Imgui_FP("y", &pVector->m4_Y); ImGui::SameLine();
    Imgui_FP("z", &pVector->m8_Z);
    ImGui::PopItemWidth();
}

void Imgui_Vec3FP(const char* name, sVec3_FP* pVector)
{
    ImGui::Text(name); ImGui::SameLine();
    ImGui::PushID(name);
    Imgui_Vec3FP(pVector);
    ImGui::PopID();
}
