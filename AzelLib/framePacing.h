#pragma once

bool framePacing_enableExperimentalNtscCadence();
bool framePacing_isExperimentalNtscCadenceEnabled();
void framePacing_waitForNextFrame(int frameLimit);
