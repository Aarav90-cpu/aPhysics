# AGENTS.md

Instructions for AI coding agents working in this repository.
Read this whole file before you change anything.

## 1. About the project

This is a physics engine for Remotion and three.js animations. It bakes in
cloth movement, crash behaviour, and other physics so that animations look
natural instead of static.

- Main language: **C**
- Compiler: **clang only**. Minimum version **23**. Do not add gcc support.
- Build system: **Bazel**. Minimum version **9**. Do not add Makefiles.
- License: Apache 2.0.

Clang and Bazel were chosen on purpose: Bazel is fast and has good clang
support, and clang is newer than gcc. Do not suggest switching.

Not decided yet (ask the user, do not guess):

- The project name.
- How the C code reaches Remotion and three.js (for example, WebAssembly).
- Which operating systems and CPU architectures are officially supported.

## 2. Working rules

### Understand first

Before you edit anything:

1. Find the code that is related to the task and read it.
2. For each function you will touch, read what it does, what calls it, what it
   calls, and what data it uses.
3. Check the surrounding code, tests, and docs for the same feature.
4. Only then decide what to change.

Do not edit code you have not read. Do not edit code to see what happens.

### Plan first

Before any change, write a short plan and show it to the user:

- What you will change and why.
- Which files and functions are affected.
- What could break.
- How you will check that it works.

Start work only after the plan is clear. If the user changes the plan,
update it.

### Ask questions

If you are unsure how something should be implemented, or what should not be
implemented, ask the user. Do not guess. Ask before you write code, not after.

### Take your time

Speed is not a goal. A small, correct, well-understood change is better than a
fast change you cannot explain. If you find that you do not fully understand
the area you are changing, stop and read more.

## 3. Build, run, and verify

The code must build and run on any supported machine, not only the user's.

- Build and test only through Bazel. Do not rely on tools installed on one
  machine.
- Pin the Bazel version in `.bazelversion`.
- Pin every external dependency to an exact version in the Bazel module files.
  Do not use "latest".
- Do not use absolute paths, usernames, or machine-specific settings in
  source, build files, or scripts.
- Do not use compiler extensions, OS-specific calls, or CPU-specific code
  without a guard and a fallback. If you must, tell the user.
- Build with warnings enabled and fix warnings instead of hiding them.
- Do not hide problems by turning off checks, deleting tests, or loosening
  assertions.

Before you report a change as finished, run the Bazel build and the Bazel
tests, and report the exact commands you ran and what they printed. If you
could not run something, say so.

You cannot prove that the code works on every machine. Only claim support for
the platforms that were actually tested, and name them.

## 4. Definition of done

Passing tests are not proof that the feature is correct.

- Code that compiles, runs, or passes tests only means those checks passed.
- Do not say "done", "fixed", or "working" unless you checked the actual
  behaviour the user asked for.
- Do not invent results, output, file contents, function names, flags, or API
  behaviour. If you do not know, say "I don't know" and go and check.
- Say clearly what you did not test.
- **Always wait for the user to confirm that the feature works** before you
  call it complete. After you finish, tell the user how to try it, then wait.

## 5. Dependencies

- Add a dependency only if it is really needed. Prefer the C standard library
  and code already in the repository.
- For each new dependency, tell the user what it is, why it is needed, its
  license, and its exact version. Wait for approval.
- Record every dependency in the Bazel module files with a pinned version, and
  in the docs.
- If something required is missing (a tool, a library, an API key, a version),
  ask the user for it. Do not work around it silently, and do not fake it.
- Remove a dependency from the build files and the docs when it is no longer
  used.

## 6. Read the docs before using any tool

Before you use or write code for any tool, library, compiler flag, or Bazel
rule:

1. Search the web for its current official documentation.
2. Read the page for the exact version this project uses.
3. Confirm that the command, flag, or function exists and does what you think.

Do not rely on memory. Tools change between versions, especially clang 23 and
Bazel 9. If the docs and your memory disagree, trust the docs.

Check any command before you run it. If you are not sure a command is correct
and safe, do not run it. Ask the user.

## 7. Code

- Write clear, structured C. Keep it small. Do not add code that is not needed.
- Do not add abstractions, wrappers, or options for cases that do not exist.
- Do not copy code. Reuse what is already there.
- Performance matters for a physics engine. Do not make code slower without a
  reason, and do not make it harder to read for an unmeasured speed gain.
  Measure before you optimise.
- Handle errors. Do not ignore return values. Do not leave undefined behaviour.
- Name functions, types, variables, and files so that their purpose is clear.
  Use one naming style across the project.

## 8. Comments and logs

Comments:

- Explain **why**, not what the code already says.
- Use plain, professional wording. No hype or filler words such as "mega",
  "ultra", "super", "blazing", or "magic".
- Keep them short and accurate. Update or delete a comment when the code
  changes.
- Write function comments in one consistent structure: purpose, inputs,
  outputs, and error cases.

Logs and messages:

- Use simple, correct English.
- Each message states what happened and, for errors, what to do about it.
- Use one consistent format, with a level (for example error, warning, info)
  and a clear source.
- Never log secrets.

## 9. Documentation

Docs must be true, short, and up to date.

- **Update the docs in the same change** whenever a feature is added, changed,
  removed, or deprecated.
- Write only what is actually true. Do not use claims like "100% secure",
  "perfect", "fully optimised", or "works everywhere". State what is
  supported, what was tested, and what the limits are.
- Explain each feature once, in one place. Link to it from elsewhere.
- Use straightforward English. Short sentences.
- Do not write long docs for small things.
- Do not document features that do not exist yet.

## 10. Agent skills for this project

When you write a skill or instruction file that tells an agent how to use this
project or one of its tools:

- Write only facts you have verified, line by line, against the code and the
  official docs.
- Use exact commands, flags, and paths. Run or check each one first.
- Use proper English and a clear format.
- Do not repeat what is already in the docs. Link to it.

## 11. License headers

Every source file must have the Apache 2.0 license header. Add it with the
`addlicense` command. Check the tool's current documentation first to get the
right flags, then run it on new files.

Do not write the header by hand, and do not remove it from existing files.

- Author/owner: `Aarav Ravindra Kharade`
- Year: `2026`

Do not change the copyright line unless explicitly authorized.

## 12. Before you finish

Go through this list and report it to the user:

- [ ] I read the related code and understood it before changing it.
- [ ] The plan was shown to the user.
- [ ] Questions I had were asked and answered.
- [ ] The code builds with Bazel and the tests ran. Commands and results are
      reported.
- [ ] Dependencies are pinned, recorded, and approved.
- [ ] No claim in my answer is something I did not check.
- [ ] Docs are updated and accurate. Nothing is repeated or exaggerated.
- [ ] Comments and logs follow section 8.
- [ ] New files have the license header.
- [ ] I have told the user how to check the feature, and I am waiting for
      their confirmation.