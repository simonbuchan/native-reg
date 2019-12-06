# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
