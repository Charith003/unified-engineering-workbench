# Automated Testing & Quality Assurance Verification Report

**Module:** Quality Assurance & Verification  
**Test Harness:** GoogleTest (gtest)  
**Binary:** `uew_tests.exe`  
**Total Executed Tests:** 45  
**Passed Tests:** 45  
**Failed Tests:** 0

---

## 1. Test Suite Architecture

The automated test suite exercises all computational modules, data parsers, and mock hardware adapters in compiled C++:

```
uew_tests.exe
├── Backprop (8 tests)
│   ├── ForwardPassInitial
│   ├── GradientVerification (Central FD vs Analytical <= 1e-5)
│   ├── GradientDirectionReducesLoss
│   ├── TrainingReducesLoss (100 steps monotonic reduction)
│   ├── LossNonNegative
│   ├── SimultaneousUpdateCorrect
│   ├── LossCSVValid
│   └── VerificationPasses
├── HardwareTest (9 tests)
│   ├── ThreeDevicesCreated
│   ├── CorrectDeviceTypes (MCU, FPGA, JTAG)
│   ├── CapabilitiesNonEmpty
│   ├── AllSimulated
│   ├── InitiallyDisconnected
│   ├── MCUConnectDisconnect
│   ├── MCUTelemetryAfterConnect
│   ├── JTAGFirstConnectFails (Simulated timeout recovery)
│   └── TelemetryOnDisconnectedThrows
├── Performance (6 tests)
│   ├── FixedSeedReproducible
│   ├── DifferentSeedsDiffer
│   ├── SerialOrderingDeterministic
│   ├── SerialParallelAgreement (<= 1e-8 °C max diff)
│   ├── BatchValuesPositive
│   └── WorkerCapRespected
├── ThermalFEM (9 tests)
│   ├── LeftBoundaryCorrect (T(0) = 100 °C)
│   ├── RightBoundaryCorrect (T(L) = 20 °C)
│   ├── AnalyticalSolutionAtMidpoint
│   ├── MeshConvergence (Monotonic error reduction across N=10,20,40,80)
│   ├── N10TheoreticalErrorExact (Matches theoretical 1.25 °C within 1e-9)
│   ├── N80TheoreticalErrorExact (Matches theoretical 0.01953125 °C within 1e-9)
│   ├── NodeCountCorrect (N+1 nodes)
│   ├── MidpointPositionsCorrect
│   └── CSVNonEmpty
├── CSVParser (6 tests)
│   ├── ValidCSVParsed
│   ├── EmptyFileRejected
│   ├── WrongHeaderRejected
│   ├── MissingColumnRejected
│   ├── NonNumericRejected
│   └── NoDataRowsRejected
└── STLParser (7 tests)
    ├── ValidSTLParsed
    ├── BoundingBoxCorrect
    ├── EmptyFileRejected
    ├── NoSolidKeywordRejected
    ├── MissingVertexRejected
    ├── NonFiniteVertexRejected (NaN / Inf rejection)
    └── NoTrianglesRejected
```

---

## 2. Actual Automated Execution Output

```
Running main() from build/_deps/googletest-src/googletest/src/gtest_main.cc
[==========] Running 45 tests from 6 test suites.
[----------] Global test environment set-up.
[----------] 8 tests from Backprop
[ RUN      ] Backprop.ForwardPassInitial
[       OK ] Backprop.ForwardPassInitial (0 ms)
[ RUN      ] Backprop.GradientVerification
[       OK ] Backprop.GradientVerification (0 ms)
[ RUN      ] Backprop.GradientDirectionReducesLoss
[       OK ] Backprop.GradientDirectionReducesLoss (0 ms)
[ RUN      ] Backprop.TrainingReducesLoss
[       OK ] Backprop.TrainingReducesLoss (0 ms)
[ RUN      ] Backprop.LossNonNegative
[       OK ] Backprop.LossNonNegative (0 ms)
[ RUN      ] Backprop.SimultaneousUpdateCorrect
[       OK ] Backprop.SimultaneousUpdateCorrect (0 ms)
[ RUN      ] Backprop.LossCSVValid
[       OK ] Backprop.LossCSVValid (0 ms)
[ RUN      ] Backprop.VerificationPasses
[       OK ] Backprop.VerificationPasses (0 ms)
[----------] 8 tests from Backprop (3 ms total)

[----------] 9 tests from HardwareTest
[ RUN      ] HardwareTest.ThreeDevicesCreated
[       OK ] HardwareTest.ThreeDevicesCreated (0 ms)
[ RUN      ] HardwareTest.CorrectDeviceTypes
[       OK ] HardwareTest.CorrectDeviceTypes (0 ms)
[ RUN      ] HardwareTest.CapabilitiesNonEmpty
[       OK ] HardwareTest.CapabilitiesNonEmpty (0 ms)
[ RUN      ] HardwareTest.AllSimulated
[       OK ] HardwareTest.AllSimulated (0 ms)
[ RUN      ] HardwareTest.InitiallyDisconnected
[       OK ] HardwareTest.InitiallyDisconnected (0 ms)
[ RUN      ] HardwareTest.MCUConnectDisconnect
[       OK ] HardwareTest.MCUConnectDisconnect (130 ms)
[ RUN      ] HardwareTest.MCUTelemetryAfterConnect
[       OK ] HardwareTest.MCUTelemetryAfterConnect (133 ms)
[ RUN      ] HardwareTest.JTAGFirstConnectFails
[       OK ] HardwareTest.JTAGFirstConnectFails (155 ms)
[ RUN      ] HardwareTest.TelemetryOnDisconnectedThrows
[       OK ] HardwareTest.TelemetryOnDisconnectedThrows (0 ms)
[----------] 9 tests from HardwareTest (420 ms total)

[----------] 6 tests from Performance
[ RUN      ] Performance.FixedSeedReproducible
[       OK ] Performance.FixedSeedReproducible (0 ms)
[ RUN      ] Performance.DifferentSeedsDiffer
[       OK ] Performance.DifferentSeedsDiffer (0 ms)
[ RUN      ] Performance.SerialOrderingDeterministic
[       OK ] Performance.SerialOrderingDeterministic (3 ms)
[ RUN      ] Performance.SerialParallelAgreement
[       OK ] Performance.SerialParallelAgreement (7 ms)
[ RUN      ] Performance.BatchValuesPositive
[       OK ] Performance.BatchValuesPositive (0 ms)
[ RUN      ] Performance.WorkerCapRespected
[       OK ] Performance.WorkerCapRespected (11 ms)
[----------] 6 tests from Performance (21 ms total)

[----------] 9 tests from ThermalFEM
[ RUN      ] ThermalFEM.LeftBoundaryCorrect
[       OK ] ThermalFEM.LeftBoundaryCorrect (0 ms)
[ RUN      ] ThermalFEM.RightBoundaryCorrect
[       OK ] ThermalFEM.RightBoundaryCorrect (0 ms)
[ RUN      ] ThermalFEM.AnalyticalSolutionAtMidpoint
[       OK ] ThermalFEM.AnalyticalSolutionAtMidpoint (0 ms)
[ RUN      ] ThermalFEM.MeshConvergence
[       OK ] ThermalFEM.MeshConvergence (1 ms)
[ RUN      ] ThermalFEM.N10TheoreticalErrorExact
[       OK ] ThermalFEM.N10TheoreticalErrorExact (0 ms)
[ RUN      ] ThermalFEM.N80TheoreticalErrorExact
[       OK ] ThermalFEM.N80TheoreticalErrorExact (0 ms)
[ RUN      ] ThermalFEM.NodeCountCorrect
[       OK ] ThermalFEM.NodeCountCorrect (0 ms)
[ RUN      ] ThermalFEM.MidpointPositionsCorrect
[       OK ] ThermalFEM.MidpointPositionsCorrect (0 ms)
[ RUN      ] ThermalFEM.CSVNonEmpty
[       OK ] ThermalFEM.CSVNonEmpty (0 ms)
[----------] 9 tests from ThermalFEM (3 ms total)

[----------] 6 tests from CSVParser
[ RUN      ] CSVParser.ValidCSVParsed
[       OK ] CSVParser.ValidCSVParsed (0 ms)
[ RUN      ] CSVParser.EmptyFileRejected
[       OK ] CSVParser.EmptyFileRejected (0 ms)
[ RUN      ] CSVParser.WrongHeaderRejected
[       OK ] CSVParser.WrongHeaderRejected (0 ms)
[ RUN      ] CSVParser.MissingColumnRejected
[       OK ] CSVParser.MissingColumnRejected (0 ms)
[ RUN      ] CSVParser.NonNumericRejected
[       OK ] CSVParser.NonNumericRejected (0 ms)
[ RUN      ] CSVParser.NoDataRowsRejected
[       OK ] CSVParser.NoDataRowsRejected (0 ms)
[----------] 6 tests from CSVParser (0 ms total)

[----------] 7 tests from STLParser
[ RUN      ] STLParser.ValidSTLParsed
[       OK ] STLParser.ValidSTLParsed (0 ms)
[ RUN      ] STLParser.BoundingBoxCorrect
[       OK ] STLParser.BoundingBoxCorrect (0 ms)
[ RUN      ] STLParser.EmptyFileRejected
[       OK ] STLParser.EmptyFileRejected (0 ms)
[ RUN      ] STLParser.NoSolidKeywordRejected
[       OK ] STLParser.NoSolidKeywordRejected (0 ms)
[ RUN      ] STLParser.MissingVertexRejected
[       OK ] STLParser.MissingVertexRejected (0 ms)
[ RUN      ] STLParser.NonFiniteVertexRejected
[       OK ] STLParser.NonFiniteVertexRejected (0 ms)
[ RUN      ] STLParser.NoTrianglesRejected
[       OK ] STLParser.NoTrianglesRejected (0 ms)
[----------] 7 tests from STLParser (0 ms total)

[----------] Global test environment tear-down
[==========] 45 tests from 6 test suites ran. (449 ms total)
[  PASSED  ] 45 tests.
```
