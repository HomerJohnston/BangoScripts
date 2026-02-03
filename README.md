# Bango Scripts

Have you ever placed a door actor, and a button actor, and wondered *"how do I make that button open that door?"* 

Have you ever placed a trigger volume and wondered *"how can I conditionally spawn three enemies at some markers, then turn on those lights, and start playing some boss music after 2 seconds?"*

Imagine if you could write a short blueprint event directly onto an actor in a level. This plugin effectively allows you to do this, by creating and attaching small "script" blueprints onto actors via a custom actor component. These "script" blueprints simply contain a `Start` event node which you execute logic from, like many traditional game script systems. ***Bango Scripts*** also provides some extra helper-nodes to bring more traditional DSL/script features into blueprints, such as explicit references to specific actors in a level.

You can create completely bespoke "level scripts" for individual actors in a level, or you can create re-usable "content scripts" in your Content folder to use in multiple places, complete with input variables configurable on each instance.

### Summary Info
> [!NOTE]
> - Intended to execute small, short-lived scripted events.
> - Supported on: UE5.6 - UE5.7
> - Current status: ⚠️**Experimental**⚠️ *some crashes and possible data loss; do not use in a real project!*
 
> [!WARNING]
> - *Runtime serialization*: Bango does *not* use custom graphs; this is ordinary Blueprint Graph code, using the normal Blueprint VM. Blueprint execution state is not serializable; you cannot natively save/restore a script back to its middle of operation (you might want to use a different system for that, like Flow Graph).
> - *Untested in multiplayer*: this plugin is currently being used by the author for single-player projects; although it just uses normal blueprints, and should follow any normal rules of blueprint, so it may work fine.

> [!CAUTION]
> - *Known issues*: Be sure to visit the [Issues](https://github.com/HomerJohnston/BangoScripts/issues) page for known problems.

&nbsp;

# Demonstration - Gameplay

https://github.com/user-attachments/assets/f2b23127-251b-479e-bb31-d9f940fdac69

The level above contains:
### ACTORS:
- Two door blueprints, each with a simple "open" event. The animation, light, and sound is all coded into the door blueprints (mostly using a Sequencer component).
- Several Light blueprints, each with a simple "turn on" event. Like the door, the animation and sound of the lights is handled by the light blueprint itself.
- Two loudspeakers (just simple actors for PlaySoundAtLocation in this case, but could be coded to start and stop some other sounds).
### TRIGGERS & SCRIPTS:
- One blank actor with a script set to auto-run. This starts the music and announcer voices coming from two loudspeakers.
- One simple pressure-plate trigger. This contains an overlap collider box and two Bango Script components (Activate/Deactivate). The scripts are ran from BeginOverlap/EndOverlap events.
- Two simple sphere collider triggers. These contain an overlap sphere and a Bango Script component. The script component is ran on a BeginOverlap event, and the trigger self-destructs to prevent repeat activations.

The trigger actors are NOT related to Bango Scripts. Your game needs to supply its own reusable trigger classes.

&nbsp;

Here's what the Bango scripts look like, in order of appearance:

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
