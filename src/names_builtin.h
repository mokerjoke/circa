// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

// This file was generated using name_tool.py

#pragma once

namespace circa {

const int name_None = 0;
const int name_File = 1;
const int name_Newline = 2;
const int name_Out = 3;
const int name_Unknown = 4;
const int name_Repeat = 5;
const int name_Success = 6;
const int name_Failure = 7;
const int name_FileNotFound = 8;
const int name_NotEnoughInputs = 9;
const int name_TooManyInputs = 10;
const int name_ExtraOutputNotFound = 11;
const int name_Default = 12;
const int name_ByDemand = 13;
const int name_Unevaluated = 14;
const int name_InProgress = 15;
const int name_Lazy = 16;
const int name_Consumed = 17;
const int name_Return = 18;
const int name_Continue = 19;
const int name_Break = 20;
const int name_Discard = 21;
const int name_InfixOperator = 22;
const int name_FunctionName = 23;
const int name_TypeName = 24;
const int name_TermName = 25;
const int name_Keyword = 26;
const int name_Whitespace = 27;
const int name_UnknownIdentifier = 28;
const int tok_Identifier = 29;
const int tok_Name = 30;
const int tok_Integer = 31;
const int tok_HexInteger = 32;
const int tok_Float = 33;
const int tok_String = 34;
const int tok_Color = 35;
const int tok_Bool = 36;
const int tok_LParen = 37;
const int tok_RParen = 38;
const int tok_LBrace = 39;
const int tok_RBrace = 40;
const int tok_LBracket = 41;
const int tok_RBracket = 42;
const int tok_Comma = 43;
const int tok_At = 44;
const int tok_Dot = 45;
const int tok_DotAt = 46;
const int tok_Star = 47;
const int tok_Question = 48;
const int tok_Slash = 49;
const int tok_DoubleSlash = 50;
const int tok_Plus = 51;
const int tok_Minus = 52;
const int tok_LThan = 53;
const int tok_LThanEq = 54;
const int tok_GThan = 55;
const int tok_GThanEq = 56;
const int tok_Percent = 57;
const int tok_Colon = 58;
const int tok_DoubleColon = 59;
const int tok_DoubleEquals = 60;
const int tok_NotEquals = 61;
const int tok_Equals = 62;
const int tok_PlusEquals = 63;
const int tok_MinusEquals = 64;
const int tok_StarEquals = 65;
const int tok_SlashEquals = 66;
const int tok_ColonEquals = 67;
const int tok_RightArrow = 68;
const int tok_LeftArrow = 69;
const int tok_Ampersand = 70;
const int tok_DoubleAmpersand = 71;
const int tok_DoubleVerticalBar = 72;
const int tok_Semicolon = 73;
const int tok_TwoDots = 74;
const int tok_Ellipsis = 75;
const int tok_TripleLThan = 76;
const int tok_TripleGThan = 77;
const int tok_Pound = 78;
const int tok_Def = 79;
const int tok_Type = 80;
const int tok_Begin = 81;
const int tok_Do = 82;
const int tok_End = 83;
const int tok_If = 84;
const int tok_Else = 85;
const int tok_Elif = 86;
const int tok_For = 87;
const int tok_State = 88;
const int tok_Return = 89;
const int tok_In = 90;
const int tok_True = 91;
const int tok_False = 92;
const int tok_Namespace = 93;
const int tok_Include = 94;
const int tok_Import = 95;
const int tok_And = 96;
const int tok_Or = 97;
const int tok_Not = 98;
const int tok_Discard = 99;
const int tok_Null = 100;
const int tok_Break = 101;
const int tok_Continue = 102;
const int tok_Switch = 103;
const int tok_Case = 104;
const int tok_While = 105;
const int tok_Whitespace = 106;
const int tok_Newline = 107;
const int tok_Comment = 108;
const int tok_Eof = 109;
const int tok_Unrecognized = 110;
const int op_NoOp = 111;
const int op_Pause = 112;
const int op_SetNull = 113;
const int op_InlineCopy = 114;
const int op_CallBranch = 115;
const int op_DynamicCall = 116;
const int op_FireNative = 117;
const int op_CaseBlock = 118;
const int op_ForLoop = 119;
const int op_ExitPoint = 120;
const int op_FinishFrame = 121;
const int op_FinishLoop = 122;
const int op_ErrorNotEnoughInputs = 123;
const int op_ErrorTooManyInputs = 124;
const int name_LoopProduceOutput = 125;
const int name_FlatOutputs = 126;
const int name_OutputsToList = 127;
const int name_Multiple = 128;
const int name_Cast = 129;
const int name_DynamicMethodOutput = 130;
const int name_FirstStatIndex = 131;
const int stat_TermsCreated = 132;
const int stat_TermPropAdded = 133;
const int stat_TermPropAccess = 134;
const int stat_InternedNameLookup = 135;
const int stat_InternedNameCreate = 136;
const int stat_Copy_PushedInputNewFrame = 137;
const int stat_Copy_PushedInputMultiNewFrame = 138;
const int stat_Copy_PushFrameWithInputs = 139;
const int stat_Copy_ListDuplicate = 140;
const int stat_Copy_LoopCopyRebound = 141;
const int stat_Cast_ListCastElement = 142;
const int stat_Cast_PushFrameWithInputs = 143;
const int stat_Cast_FinishFrame = 144;
const int stat_Touch_ListCast = 145;
const int stat_ValueCreates = 146;
const int stat_ValueCopies = 147;
const int stat_ValueCast = 148;
const int stat_ValueCastDispatched = 149;
const int stat_ValueTouch = 150;
const int stat_ListsCreated = 151;
const int stat_ListsGrown = 152;
const int stat_ListSoftCopy = 153;
const int stat_ListHardCopy = 154;
const int stat_DictHardCopy = 155;
const int stat_StringCreate = 156;
const int stat_StringDuplicate = 157;
const int stat_StringResizeInPlace = 158;
const int stat_StringResizeCreate = 159;
const int stat_StringSoftCopy = 160;
const int stat_StringToStd = 161;
const int stat_StepInterpreter = 162;
const int stat_InterpreterCastOutputFromFinishedFrame = 163;
const int stat_BranchNameLookups = 164;
const int stat_PushFrame = 165;
const int stat_LoopFinishIteration = 166;
const int stat_LoopWriteOutput = 167;
const int stat_WriteTermBytecode = 168;
const int stat_DynamicCall = 169;
const int stat_FinishDynamicCall = 170;
const int stat_DynamicMethodCall = 171;
const int stat_SetIndex = 172;
const int stat_SetField = 173;
const int name_LastStatIndex = 174;
const int name_LastBuiltinName = 175;

const char* builtin_name_to_string(int name);

} // namespace circa
