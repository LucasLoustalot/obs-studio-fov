# OBS Studio for FOV

This repository contains the customized distribution of OBS Studio optimized for integration with the Flexible Output View (FOV) ecosystem.

For the official, unmodified upstream source, refer to the [upstream OBS Studio repository](https://github.com/obsproject/obs-studio).

## Installation
Precompiled binaries are available via the [GitHub Releases page](https://github.com/Flexible-Output-View/obs-studio-fov/releases).

## Compilation and Deployment

### Repository Cloning
To clone the source repository along with all nested dependencies and submodules, execute the following command:

```bash
git clone --recurse-submodules git@github.com:Flexible-Output-View/obs-studio-fov.git
```

### Automation Scripts
Manual compilation and system deployment on Ubuntu environments are executed via the provided automation shell scripts:
- `requirements.sh`: Installs mandatory system dependency packages. This script must be executed prior to compilation.
- `build_install_release_ubuntu.sh`: Performs a clean release compilation and installs the resulting binaries to the host system. Clears the build cache prior to execution.
- `build_install_ubuntu.sh`: Performs a clean debug compilation and installs the resulting binaries to the host system. Clears the build cache prior to execution.
- `quick_build_install_ubuntu.sh`: Executes an optimized incremental release compilation and installation sequence.
- `build_portable_ubuntu.sh`: Compiles a portable execution binary without modifying system directories.

## Quick Start
Operation requires an active deployment of the [web-fov backend platform](https://github.com/Flexible-Output-View/web-fov) or access to a public service endpoint (e.g., [https://fovapp.live](https://fovapp.live)).

### Service Configuration
To route multi-track streams to an FOV instance, navigate to the OBS Settings interface, select the **FOV - Multitrack** service, and configure the following parameters:
- **URL**: The API endpoint of the target instance (e.g., `https://api.fovapp.live`).
- **Stream key**: The assigned session authentication token. *(Note: This feature is under active development; currently, any unique identifier string is accepted).*

Initiate transmission by selecting **Start Streaming** within the main application user interface.

![Service Configuration](./docs/fov-service-image.png)

> [!Note]
> - All active video sources present within the active scene are encoded as discrete, independent video tracks.
> - Audio allocation across sub-streams is determined by the track routing matrix established within the OBS Advanced Audio Properties interface (e.g., Mix 1, Mix 2).
> - Dynamic modification (addition or deletion) of video sources is prohibited during an active streaming session.
> - In the event of initialization or transport failures, extract and submit the system log files for diagnostic analysis.

> [!Warning]
> - Concurrent multi-source encoding generates significant processing overhead. The utilization of hardware-accelerated encoders (NVENC, AMF, QuickSync) is strictly required to prevent system resource exhaustion.
> - Encoding parameters are uniformly enforced across all active tracks. Individual track bitrates must be restricted to remain within cumulative system and network ingestion thresholds.
> - Multi-track ingestion increases cumulative outbound network bandwidth utilization linearly relative to the total track count compared to a standard single-track stream.

## Contribution Guidelines

### Bug Tracking and Feature Requests
> [!Warning]
> - Issue tracking is strictly centralized on the [OBS/FOV repository](https://github.com/Flexible-Output-View/obs-studio-fov).
> - Issues unrelated to custom FOV features are subject to immediate closure.
> - Defect reports must contain comprehensive environmental context, configuration specifics, and deterministic reproduction steps.

To facilitate diagnostics, include hardware/software configurations and the native OBS log file (accessible via the Help menu in the application toolbar) when opening a defect report.

![Show the log file](./docs/log_file.png)

Structural feature proposals and optimization discussions are hosted on the [GitHub Discussions forum](https://github.com/orgs/Flexible-Output-View/discussions/1).

### Pull Requests
Code contributions must be submitted via Pull Requests detailing the technical scope of the proposed changes. Submissions must comply with the upstream [OBS Contribution Guidelines](./CONTRIBUTING.rst) subject to the following project-specific requirements:
- **Commit Formatting**: Commits must conform to the following prefix convention: `ADD/UPDATE/DELETE/CHORE/FIX: <Description>`.
- **Validation and Testing**: Because upstream static analysis tooling is unavailable outside the primary OBS repository, contributors must perform local validation and regression testing prior to submission.
- **Architectural Modification**: Changes impacting core system architecture, ingestion protocols, or the wider web-fov ecosystem require prior authorization via GitHub Discussions. Submissions must explicitly tag the core maintainers: @LucasLoustalot, @RaphxelS, and @Slymoz.
