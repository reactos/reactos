This directory contains MilCodeGen, which uses the Csp tool to 
generate source code for the MIL resources.



csp.exe will be built automatically, but if you want to build
it by hand:
    Do "b" in %sdxroot%\wpf\src\Graphics\tools\csp


To run MilCodeGen:
    Do "b" in this directory


To run MilCodeGen under Rascal: 
    Make sure csp.exe is built
    Run tools\DebugMCG.cmd GenerateFiles.cmd




Control flow in these templates is as follows:
 * Execution starts at ResourceGenerator.Main()
 * That method invokes the Go() method on each subclass of GeneratorBase. 
   i.e. it invokes methods like ManagedResource.Go() and DataStruct.Go().
