#	$NetBSD: Makefile,v 1.12 2011/12/19 14:07:40 joerg Exp $

LIB=	LLVMCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/CodeGen

SRCS+=	AggressiveAntiDepBreaker.cpp \
	AllocationOrder.cpp \
	Analysis.cpp \
	BranchFolding.cpp \
	CalcSpillWeights.cpp \
	CallingConvLower.cpp \
	CodeGen.cpp \
	CodePlacementOpt.cpp \
	CriticalAntiDepBreaker.cpp \
	DeadMachineInstructionElim.cpp \
	DwarfEHPrepare.cpp \
	EdgeBundles.cpp \
	ELFCodeEmitter.cpp \
	ELFWriter.cpp \
	ExecutionDepsFix.cpp \
	ExpandISelPseudos.cpp \
	ExpandPostRAPseudos.cpp \
	GCMetadata.cpp \
	GCMetadataPrinter.cpp \
	GCStrategy.cpp \
	IfConversion.cpp \
	InlineSpiller.cpp \
	InterferenceCache.cpp \
	IntrinsicLowering.cpp \
	LLVMTargetMachine.cpp \
	LatencyPriorityQueue.cpp \
	LexicalScopes.cpp \
	LiveDebugVariables.cpp \
	LiveInterval.cpp \
	LiveIntervalAnalysis.cpp \
	LiveIntervalUnion.cpp \
	LiveRangeCalc.cpp \
	LiveStackAnalysis.cpp \
	LiveVariables.cpp \
	LiveRangeEdit.cpp \
	LocalStackSlotAllocation.cpp \
	MachineBasicBlock.cpp \
	MachineBranchProbabilityInfo.cpp \
	MachineBlockFrequencyInfo.cpp \
	MachineBlockPlacement.cpp \
	MachineCSE.cpp \
	MachineDominators.cpp \
	MachineFunction.cpp \
	MachineFunctionAnalysis.cpp \
	MachineFunctionPass.cpp \
	MachineFunctionPrinterPass.cpp \
	MachineInstr.cpp \
	MachineInstrBundle.cpp \
	MachineLICM.cpp \
	MachineLoopInfo.cpp \
	MachineLoopRanges.cpp \
	MachineModuleInfo.cpp \
	MachineModuleInfoImpls.cpp \
	MachinePassRegistry.cpp \
	MachineRegisterInfo.cpp \
	MachineSSAUpdater.cpp \
	MachineSink.cpp \
	MachineVerifier.cpp \
	ObjectCodeEmitter.cpp \
	OcamlGC.cpp \
	OptimizePHIs.cpp \
	PHIElimination.cpp \
	PHIEliminationUtils.cpp \
	Passes.cpp \
	PeepholeOptimizer.cpp \
	ScoreboardHazardRecognizer.cpp \
	PostRASchedulerList.cpp \
	ProcessImplicitDefs.cpp \
	PrologEpilogInserter.cpp \
	PseudoSourceValue.cpp \
	RegAllocBasic.cpp \
	RegAllocFast.cpp \
	RegAllocGreedy.cpp \
	RegAllocPBQP.cpp \
	RegisterClassInfo.cpp \
	RegisterCoalescer.cpp \
	RegisterScavenging.cpp \
	RenderMachineFunction.cpp \
	ScheduleDAG.cpp \
	ScheduleDAGEmit.cpp \
	ScheduleDAGInstrs.cpp \
	ScheduleDAGPrinter.cpp \
	ShadowStackGC.cpp \
	ShrinkWrapping.cpp \
	SjLjEHPrepare.cpp \
	SlotIndexes.cpp \
	Spiller.cpp \
	SpillPlacement.cpp \
	SplitKit.cpp \
	StackProtector.cpp \
	StackSlotColoring.cpp \
	StrongPHIElimination.cpp \
	TailDuplication.cpp \
	TargetFrameLoweringImpl.cpp \
	TargetInstrInfoImpl.cpp \
	TargetLoweringObjectFileImpl.cpp \
	TargetOptionsImpl.cpp \
	TwoAddressInstructionPass.cpp \
	UnreachableBlockElim.cpp \
	VirtRegMap.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
# Crashes some versions of GCC
CPPFLAGS.RenderMachineFunction.cpp+= -O0
.else
.include <bsd.lib.mk>
.endif
