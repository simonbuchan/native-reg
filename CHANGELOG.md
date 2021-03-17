# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Some [examples]() showing usage.

### Changed

- Updated dependencies.

### Fixed

- [#8](https://github.com/simonbuchan/native-reg/issues/8): Removes `os` dependency and makes this package
  assert if called on non-windows platforms. Thanks to [@aabuhijleh](https://github.com/aabuhijleh) for solution here.

## [v0.3.4] - 2020-03-06

### Fixed

- [#7](https://github.com/simonbuchan/native-reg/issues/7): Nasty crash in `getValue()` on later versions of v8 on some values.

## [v0.3.3] - 2019-12-11

Note this release has potentially breaking changes, to reduce the
potential exposure to a nasty bug in the earlier v0.3 releases.

Standard usage should not be affected.

### Changed
 
- The allocated key values can no longer be an `External` value
  that can be passed to other packages, they are now instances
  of an (internal) HKEY type that have a `native` property that
  has the previous `External` value, or `null` after they are closed.
- Use of closed keys will therefore no longer throw the underlying
  windows API invalid value, but instead (for now) a standard `Error`
  with the `message` of `"HKEY already closed"`.

### Fixed

- Nasty double-free with `closeKey()`. Was always possible if
  it was used incorrectly before, but `v0.3.0` introduced
  GC, which means it was not possible to correctly use
  `closeKey()`. Huge thanks to [@cwalther](https://github.com/cwalther)
  for catching and fixing this!

## [v0.3.2] - 2019-12-06

### Fixed

- Running on 32-bit node. Prebuilds both x86 and x64,
  don't try to fall back since it would currently fail
  (due to not depending on `node-addon-api`).

## [v0.3.1] - 2019-12-06

Broken

## [v0.3.0] - 2019-12-06

Minor release due to there being significant internal code changes.

### Added

- Use [prebuildify](https://github.com/prebuild/prebuildify) to
  package a win64 prebuild.

### Changed

- Moved to use N-API so we can prebuild binaries, big thanks to [@danielgindi](https://github.com/danielgindi)

### Fixed

- Running on node < 10, from [@cwalther](https://github.com/cwalther)
- Correctly strip empty strings from the raw string `"\0\0"` to `""`, from [@cwalther](https://github.com/cwalther).  
  Affects `getValue()`, `queryValue()`, `parseString()`, `parseValue()`.

## [v0.2.1] - 2018-10-02

First complete version.
