# Online play (experimental)

Status: in testing. Two copies of the game on one PC, and two PCs through Steam, have played songs together, joined by
invites.

The game's online modes, played between friends. There are no Xbox Live or game servers. Each player's game
connects to the others, and everyone needs the same version of this port. The online stack is GH Metallica's (the
SDK's online changes 89-103), with more SDK changes and four hooks for this game.

## Setting it up

In the launcher, open **Settings > Online** and turn **Online play** on, on every PC. **Through Steam** turns on with it.

- Steam must be running and signed in on every PC.
- The players must be Steam friends.
- Valve's `steam_api64.dll` must be next to the game (the release includes it).
- Steam shows the game as Spacewar (app 480).
- Steam carries the game's traffic between the players through its relays, so no port needs to be opened.

The options are the same as GH Metallica's (its `docs/online-play.md`): `online_play`, `online_steam`, `online_host`,
`online_port`, `online_upnp` and `online_xuid`, written to `ghwor_options.toml`.

## In the game

With online play on, every band lobby is an online party from the start (Quickplay+, competitive, Quest), as on a
console signed in to Xbox Live. Friends can join at any time:

- from the lobby's **FRIENDS LIST** (an invite: a card at the top of the screen, START held for 2 seconds to join);
- from Steam's friends list (**Invite to game** / **Join game**).

The lobby's orange button looks for players outside the party. A friend in the party is shown as a silhouette
("Player connected"), as on the console.

The game does not ask about Facebook or Twitter the first time a profile goes online: those services are gone
(see 4 below).

## What was changed for this game

### 1. The party's key (SDK change 106)

- **Symptom.** A friend who joined the party saw "Cannot join the Xbox LIVE party. Session no longer available".
- **Cause.** This game never registers its session's key with XNet. The XDK's session library did that on the
  console, so the virtual network did not know the party and did not answer the friend's QoS probe.
- **Fix.** XSessionCreate registers the key.

### 2. Socket numbers (SDK change 107)

- **Symptom.** The same message, 20 seconds after the friend had joined the party's session.
- **Cause.** The game's network layer (Demonware 2.3's bdNet, VDP port 1000) treated the socket it was given as a
  failed `socket()` call. The SDK gave kernel handles (0xF8......), which are negative as signed numbers, so bdNet never
  bound its port and sent nothing.
- **Fix.** The game is given small positive socket numbers, as on the console.

### 2b. No port without online play (SDK change 108)

- **Cause.** With change 107, bdNet also bound its socket with online play off. That socket was a real UDP socket,
  so every start opened UDP port 1000 on the PC (and could bring up the firewall's question) for nothing.
- **Fix.** Without online play, a VDP socket has no network, as on a console without a cable: nothing is opened on the
  PC. Checked: Warriors of Rock, GH Metallica, Guitar Hero 5 and Band Hero open no port offline.

### 2c. The friends list (SDK change 109)

- **Symptom.** A friend who started their game after yours never showed in your FRIENDS LIST, so you could not
  invite them.
- **Cause.** This game reads its friends list once, when it starts, and again only when the console tells it that a
  friend's state changed. The SDK never told it.
- **Fix.** The SDK tells the game whenever its friends list changes: a friend connects or leaves, or hosts or leaves a
  party.

### 2d. After the host quits a song (SDK change 111)

- **Symptom.** The host quit a song, and a friend who had joined by an invite went back to a lobby of their own, then
  into the host's new party, with nobody pressing anything.
- **Cause.** The SDK takes a friend who joined without an invite to the host's party once. An accepted invite did not
  count as that once, so it happened later, at the host's next party.
- **Fix.** An accepted invite counts. After the host's party is gone, each player stays in their own lobby until
  invited again, as before.

### 3. The matchmaking's wait (`ghwor_online_host_wait`, 3)

- **Cause.** The matchmaker hosts each match for strangers to fill its empty places: 15 seconds plus up to 4, and up
  to 30 in all.
- **Fix.** It waits 3 seconds (`src/matchmaking.cpp`).
- **To turn it off.** 0 keeps the game's own times, for two groups of friends that want to find each other.

### 4. The texts (`ghwor_online_text`, true)

- **Fix.** The lobby's "FIND Xbox LIVE PLAYERS" says "FIND ONLINE PLAYERS" ("BUSCAR JUGADORES EN LÍNEA", ...). Five more
  lobby texts that name Xbox LIVE show the game's own PlayStation 3 words, and the Options menu has no Xbox LIVE Party
  item (the console's party chat). See `src/online_text.cpp`.
- **The sharing question.** At a profile's first online start the game waited up to 6 seconds for Activision's
  servers, then asked "Do you want to login to your Facebook or Twitter accounts to share?". The developers' own skip
  for that screen (`g_skip_social_networks`) is turned on, so the game goes straight to the main menu. The Options
  menu's Share item is unchanged.
- **What stays.** Texts without a neutral PS3 twin still say Xbox LIVE (the downloadable content warning, sign-in
  messages).
- **To turn it off.** `false` brings back the console's texts, the item and the question.

### 5. Invites during a song

- **Fix.** An invite accepted during a song or a loading screen reaches the game only once it is back in the menus.
  In GH Metallica, an invite delivered while its menus were loading crashed the game.

## Not offered

Several features depend on Activision's or Microsoft's servers, and they fail as they do offline:

- the music store;
- Rock Record, leaderboards and GHTunes;
- Facebook and Twitter sharing;
- the Xbox LIVE Party.

The party's host migration (a new host when the host leaves with 3 or more players) is off in this game: no script
turns it on, and its XDK call is never made. When the host leaves, each player goes back to a lobby of their own.

## Turning it off

**Online play** off (the default): the game is offline, as before.
