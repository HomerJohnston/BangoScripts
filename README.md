# Bango Scripts

Have you ever placed a trigger volume and wondered *"how can I conditionally spawn three enemies at some markers, then turn on those lights, and start playing some boss music after 2 seconds?"*

Imagine if you could write a short blueprint event directly onto an actor in a level. This plugin effectively allows you to do this. You can create small "script" blueprints on actors via a custom actor component. These "script" blueprints contain a single `Start` event node. Running the script component will spawn and run an instance of the script blueprint. 

***Bango Scripts*** also provides some extra helper-nodes which bring more traditional DSL/script features into blueprints, such as explicit references to specific actors in a level (similar to what Level Blueprints can do, but better).

You can also create re-usable scripts in your Content folder to use on multiple  actors, with input variables which are configurable on each instance.

### Summary Info
> [!NOTE]
> - Intended to execute small, short-lived scripted events.
> - Supported on: UE5.6 - UE5.7
> - Current status: ⚠️**Experimental**⚠️ *some crashes and possible data loss; do not use in a real project!*
 
> [!WARNING]
> - *Runtime serialization*: Bango uses Blueprint. Blueprint graph execution state is not serializable; you cannot simply save/restore a script back to its middle of operation (you might want to use a different system for that, like Flow Graph).
> - *Untested in multiplayer*: this plugin is currently being used for single-player projects; it may work fine, but is untested in multiplayer.

> [!CAUTION]
> - *Known issues*: Be sure to visit the [Issues](https://github.com/HomerJohnston/BangoScripts/issues) page for known problems.

&nbsp;

# Demonstration - Gameplay

https://github.com/user-attachments/assets/f2b23127-251b-479e-bb31-d9f940fdac69

The level above contains:
### ACTORS:
- Two doors, each with a simple "open" event. The animation, light, and sound is all handled in the door blueprint.
- Several lights, each with a simple "turn on" event. Like the door, the animation and sound of the lights is handled in the light blueprint.
- Two loudspeakers, just dummy actors for using PlaySoundAtLocation.
- 
### TRIGGERS & SCRIPTS:
- One blank actor with a script, set to auto-run. This starts the music and announcer voices coming from two loudspeakers.
- One pressure-plate trigger controlling the first door. This contains an overlap collider box and two Bango Script components (Activate/Deactivate). The Activate script runs on BeginOverlap, and vice versa.
- Two simple sphere collider triggers, the first turning on the hallway lights and the second opening the last door. These contain an overlap sphere and a Bango Script component. The script component is ran on the trigger's BeginOverlap event. The trigger is coded to self-destruct, to prevent repeat activations.

The trigger actors are NOT related to Bango Scripts. Your game needs to supply its own reusable trigger classes.

&nbsp;

Here's what the scripts for this scene look like, in order of appearance:

&nbsp;

<img width="1910" height="1068" alt="image" src="https://github.com/user-attachments/assets/739bfc2a-4b73-423b-90a7-c6e27792e438" />

<p align="center"><i>The startup script. This plays PA system audio and starts music for the scene. </i></p>

&nbsp;

<img width="1908" height="1069" alt="image" src="https://github.com/user-attachments/assets/f38f90f5-57d1-4bd1-9d0e-4a7a9bc1e33b" />

<p align="center"><i>The pressure plate "Activate" script. This opens and closes the first door. It also has a separate "Deactivate" script which closes the door. </i></p>

&nbsp;

<img width="1904" height="1076" alt="image" src="https://github.com/user-attachments/assets/7eed9bf3-e41c-42d9-8f02-ccc402dd0f0c" />

<p align="center"><i>The first spherical trigger. This turns on all of the lights, using some various delays.</i></p>

&nbsp;

<img width="1909" height="1077" alt="image" src="https://github.com/user-attachments/assets/6ed47123-a415-4f21-8102-7a48c123d979" />

<p align="center"><i>The third trigger, which simply opens the last door.</i></p>

&nbsp;

# Getting Started

Go to the wiki to get started using Bango Scripts.
