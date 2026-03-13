# COOL-compiler
**Authors:** Stefan Stoyanov (me), Eli Trumbeva (@EliTrumbeva)
As simple as possible working compiler for the programming language COOL (Classroom Objected Oriented Language) in terms of functionality. We coded it in order to complete a university course for compilers at Sofia University, Faculty of Mathematics and Informatics. The course in SU is based on the Standford course CS143. However, our task was to built compiler which genereate RISC-V assembly code compared to CS143's task to generate MIPS assembly code. 

In addition, our code can only work with template code which can be found on https://github.com/Aristotelis2002/uni-coolc.

Our lexer, parser and semantic analyxer pass all of the provided tests while the code generator passes only 96 of 100. It is noteworthy that the code generator lacks the check of dispathching a method on a void object (uninitialized) because there weren't such provided tests at the time when we wrote the code generator.

We used ANTLR4 as a lexer and parser.
Our compiler lack optimizations of any kind.
