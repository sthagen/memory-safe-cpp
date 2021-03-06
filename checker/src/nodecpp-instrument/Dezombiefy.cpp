/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#include "Dezombiefy.h"

#include "CodeChange.h"
#include "Dezombify1ASTVisitor.h"
#include "Dezombify2ASTVisitor.h"
#include "DezombiefyRelaxASTVisitor.h"


namespace nodecpp {

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

void DezombiefyStats::printStats() {
  
  llvm::errs() << "Dezombiefy stats Vars:" << VarCount << ", This:" <<
    ThisCount << ", Relaxed:" << RelaxedCount << "\n";
}


void dezombiefy(ASTContext &Ctx, bool SilentMode) {
      
  Dezombify1ASTVisitor Visitor1(Ctx, SilentMode);
  Visitor1.TraverseDecl(Ctx.getTranslationUnitDecl());

  DezombiefyRelaxASTVisitor VisitorRelax(Ctx, SilentMode);
  VisitorRelax.TraverseDecl(Ctx.getTranslationUnitDecl());

  Dezombify2ASTVisitor Visitor2(Ctx, SilentMode);
  Visitor2.TraverseDecl(Ctx.getTranslationUnitDecl());

  Visitor2.getStats().printStats();

  auto &Reps = Visitor2.finishReplacements();
  overwriteChangedFiles(Ctx, Reps, "nodecpp-dezombiefy");
}


} //namespace nodecpp