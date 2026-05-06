
"""
Puffle Rescue - Pygame Zero
Run with: pgzrun puffle_rescue.py

Controls:
  Arrow Keys - Move / Jump
  Space      - Restart after win/loss
"""

"""
Pico(2):
GP14: Left
GP15: Right
GP16: Jump

Remember to install pyserial
"""


import pgzrun
import math
import pygame
import array

#Pico Set up 
USE_PICO = True
PICO_PORT = '/dev/cu.usbmodem101'  # ← change to your port
BAUD_RATE = 115200
#Open serial port
_ser = None
if USE_PICO:
    try:
        import serial
        _ser = serial.Serial(PICO_PORT, BAUD_RATE, timeout=0.01)
        print(f"Pico connected on {PICO_PORT}")
    except Exception as e:
        print(f"Could not open serial port: {e}")
        print("Falling back to keyboard input.")
        _ser = None
 

# ── Window ────────────────────────────────────────────────────────────────────
WIDTH  = 430
HEIGHT = 600
TITLE  = "Puffle Rescue"

# ── Colours ───────────────────────────────────────────────────────────────────
BG_COLOR       = (10,  15,  40)
PLATFORM_COLOR = (200, 50,  50)
PLAYER_COLOR   = (80,  200, 230)
PUFFLE_COLOR   = (255, 180,  60)
TEXT_COLOR     = (255, 255, 255)
DIM_COLOR      = (160, 160, 160)
WIN_COLOR      = (80,  230, 120)
FLASH_COLOR    = (255, 60,  60)

# ── Physics ───────────────────────────────────────────────────────────────────
GRAVITY     =  0.55
JUMP_FORCE  = -13.0
MOVE_SPEED  =  4.0

# ── Wall flash timing ─────────────────────────────────────────────────────────
FLASH_DURATION = 30   # frames (~0.5 s at 60 fps)

# ── Platform layout  (x, y, width, height, speed, fixed) ─────────────────────
# speed = horizontal pixels/frame; fixed=True means it never moves
# Puffle platform (level 5 right, x=170) is fixed.
PLATFORM_DEFS = [
    # floor
    (0,   570, 430, 30,  0.0,  True),
    # level 1
    (0,   490, 170, 14,  1.2, False),
    (260, 490, 170, 14, -1.0, False),
    # level 2
    (80,  415, 200, 14,  0.9, False),
    (330, 415, 100, 14, -1.3, False),
    # level 3
    (0,   340, 130, 14,  1.1, False),
    (230, 340, 200, 14, -0.8, False),
    # level 4
    (60,  265, 220, 14,  1.0, False),
    (350, 265,  80, 14, -1.4, False),
    # level 5  left (moves)
    (0,   190, 100, 14,  1.2, False),
    # level 5  right — FIXED, puffle sits here
    (170, 190, 180, 14,  0.0,  True),
]

# Puffle sits on top of the fixed level-5 right platform
PUFFLE_X = 170 + 90   # centre = 260
PUFFLE_Y = 190 - 14

# ── Chiptune win sound (pure pygame, no numpy) ────────────────────────────────
SAMPLE_RATE = 22050

def _make_chiptune_win():
    """
    A short 4-note chiptune arpeggio: C5 E5 G5 C6, each 0.12 s,
    square-wave timbre for that retro video-game feel.
    """
    notes = [523, 659, 784, 1047]   # C5 E5 G5 C6
    note_dur = 0.12
    samples_per_note = int(SAMPLE_RATE * note_dur)
    buf = array.array("h")

    for freq in notes:
        period = SAMPLE_RATE / freq
        for i in range(samples_per_note):
            env = max(0.0, 1.0 - (i / samples_per_note) * 0.3)
            # square wave: +1 or -1 depending on phase
            phase = (i % period) / period
            val = int(20000 * env * (1 if phase < 0.5 else -1))
            buf.append(val)
            buf.append(val)   # stereo

    raw = bytes(buf)
    sound = pygame.sndarray.make_sound(
        pygame.sndarray.samples(
            pygame.mixer.Sound(buffer=raw)
        )
    )
    # The above is a round-trip trick; simpler direct path:
    sound = pygame.mixer.Sound(buffer=raw)
    return sound

_win_sound = None

def _play_win_sound():
    global _win_sound
    try:
        pygame.mixer.pre_init(SAMPLE_RATE, -16, 2, 512)
        if _win_sound is None:
            _win_sound = _make_chiptune_win()
        _win_sound.play()
    except Exception:
        pass

# ── Platform objects ──────────────────────────────────────────────────────────
class Platform:
    def __init__(self, x, y, w, h, speed, fixed):
        self.x     = float(x)
        self.y     = float(y)
        self.w     = w
        self.h     = h
        self.speed = speed          # current signed velocity
        self.fixed = fixed
        self.rect  = Rect(int(x), int(y), w, h)

    def update(self):
        if self.fixed:
            return
        self.x += self.speed
        # Bounce off screen edges
        if self.x < 0:
            self.x    = 0
            self.speed = abs(self.speed)
        elif self.x + self.w > WIDTH:
            self.x    = WIDTH - self.w
            self.speed = -abs(self.speed)
        self.rect = Rect(int(self.x), int(self.y), self.w, self.h)

# ── State ─────────────────────────────────────────────────────────────────────
attempts      = 0
_space_was_held = False
flash_timer   = 0   # > 0 means we're in the wall-flash countdown

def make_player():
    return {
        "x":   195.0,
        "y":   530.0,
        "vx":  0.0,
        "vy":  0.0,
        "w":   22,
        "h":   22,
        "on_ground": False,
    }

def make_platforms():
    return [Platform(x, y, w, h, spd, fx)
            for (x, y, w, h, spd, fx) in PLATFORM_DEFS]

player     = make_player()
platforms  = make_platforms()
game_state = "playing"   # "playing" | "win" | "flash"

# ── Input ─────────────────────────────────────────────────────────────────────
def get_input() -> dict:
    """
    Returns {"left", "right", "jump", "restart"} as booleans.
    """
    # Pico 2 hardware
    if _ser is not None:
        try:
            line  = _ser.readline().decode('utf-8').strip()
            # Format: "(left,right,jump)"  →  e.g. "(1,0,1)"
            inner = line[line.find('(') + 1 : line.find(')')]
            parts = inner.split(',')
            return {
                "left":    bool(int(parts[0])),
                "right":   bool(int(parts[1])),
                "jump":    bool(int(parts[2])),
                "restart": False,   # Space on keyboard still restarts
            }
        except Exception:
            pass   # bad/empty read → fall through to keyboard
 
    # Keyboard fallback (also always handles restart/Space)
    keys = pygame.key.get_pressed()
    return {
        "left":    bool(keys[pygame.K_LEFT]),
        "right":   bool(keys[pygame.K_RIGHT]),
        "jump":    bool(keys[pygame.K_UP]),
        "restart": bool(keys[pygame.K_SPACE]),
    }


# ── Helpers ───────────────────────────────────────────────────────────────────
def player_rect(p):
    return Rect(int(p["x"] - p["w"] / 2),
                int(p["y"] - p["h"] / 2),
                p["w"], p["h"])

def reset_game():
    global player, game_state, attempts, flash_timer, platforms
    player     = make_player()
    platforms  = make_platforms()
    game_state = "playing"
    flash_timer = 0
    attempts  += 1

def check_win(p):
    pr       = player_rect(p)
    puffle_r = Rect(PUFFLE_X - 14, PUFFLE_Y - 14, 28, 28)
    return pr.colliderect(puffle_r)

# ── Update ────────────────────────────────────────────────────────────────────
def update():
    global game_state, _space_was_held, flash_timer

    inp = get_input()
    space_just_pressed = inp["restart"] and not _space_was_held
    _space_was_held    = inp["restart"]

    # ── Flash state: count down then restart ──────────────────────────────────
    if game_state == "flash":
        flash_timer -= 1
        if flash_timer <= 0:
            reset_game()
        return

    # ── Win state ─────────────────────────────────────────────────────────────
    if game_state == "win":
        if space_just_pressed:
            reset_game()
        return

    # Space restart during play
    if space_just_pressed:
        reset_game()
        return

    # ── Move platforms ────────────────────────────────────────────────────────
    for plat in platforms:
        plat.update()

    # ── Player physics ────────────────────────────────────────────────────────
    p = player

    p["vx"] = 0.0
    if inp["left"]:
        p["vx"] = -MOVE_SPEED
    if inp["right"]:
        p["vx"] =  MOVE_SPEED

    if inp["jump"] and p["on_ground"]:
        p["vy"] = JUMP_FORCE

    p["vy"] += GRAVITY

    # Move X
    p["x"] += p["vx"]

    # ── Wall collision → flash then restart ───────────────────────────────────
    half_w = p["w"] / 2
    if p["x"] - half_w < 0 or p["x"] + half_w > WIDTH:
        p["x"]      = max(half_w, min(WIDTH - half_w, p["x"]))
        game_state  = "flash"
        flash_timer = FLASH_DURATION
        return

    # Move Y with platform collision
    p["y"] += p["vy"]
    p["on_ground"] = False

    pr = player_rect(p)
    for plat in platforms:
        if pr.colliderect(plat.rect):
            if p["vy"] > 0 and pr.bottom - p["vy"] <= plat.rect.top + 4:
                p["y"]         = plat.rect.top - p["h"] / 2
                p["vy"]        = 0
                p["on_ground"] = True
            elif p["vy"] < 0 and pr.top - p["vy"] >= plat.rect.bottom - 4:
                p["y"]  = plat.rect.bottom + p["h"] / 2
                p["vy"] = 0
            else:
                if p["vx"] > 0:
                    p["x"] = plat.rect.left  - p["w"] / 2
                elif p["vx"] < 0:
                    p["x"] = plat.rect.right + p["w"] / 2
            pr = player_rect(p)

    # Fell off bottom
    if p["y"] > HEIGHT + 40:
        reset_game()
        return

    # Win check
    if check_win(p):
        game_state = "win"
        _play_win_sound()

# ── Draw ──────────────────────────────────────────────────────────────────────
def draw():
    # Flash overlay: red bg during flash state
    if game_state == "flash":
        intensity = int(255 * (flash_timer / FLASH_DURATION))
        screen.fill((min(255, 80 + intensity), 15, 40))
        screen.draw.text("WALL HIT!",
                         center=(WIDTH // 2, HEIGHT // 2),
                         color=(255, 220, 220), fontsize=48)
        screen.draw.text(f"Attempts: {attempts}",
                         topleft=(8, 8),
                         color=DIM_COLOR, fontsize=22)
        return

    screen.fill(BG_COLOR)

    # Platforms
    for plat in platforms:
        screen.draw.filled_rect(plat.rect, PLATFORM_COLOR)
        screen.draw.line((plat.rect.left,  plat.rect.top),
                         (plat.rect.right, plat.rect.top), (230, 90, 90))

    # Puffle
    screen.draw.filled_circle((PUFFLE_X, PUFFLE_Y), 14, PUFFLE_COLOR)
    screen.draw.circle((PUFFLE_X, PUFFLE_Y), 14, (200, 130, 20))
    screen.draw.filled_circle((PUFFLE_X + 5, PUFFLE_Y - 4), 3, (30, 10, 10))

    # Player
    pr = player_rect(player)
    screen.draw.filled_rect(pr, PLAYER_COLOR)
    screen.draw.rect(pr, (40, 160, 200))

    # HUD
    input_label = "Pico 2" if _ser is not None else "Keyboard"
    screen.draw.text(f"Attempts: {attempts}",
                     topleft=(8, 8),
                     color=DIM_COLOR, fontsize=22)
    screen.draw.text("↑ Jump   ← → Move   Space Restart",
                     topleft=(8, HEIGHT - 24),
                     color=DIM_COLOR, fontsize=16)

    # Win overlay
    if game_state == "win":
        banner = Rect(60, 220, 310, 130)
        screen.draw.filled_rect(banner, (10, 30, 20))
        screen.draw.rect(banner, WIN_COLOR)
        screen.draw.text("PUFFLE RESCUED!",
                         center=(WIDTH // 2, 260),
                         color=WIN_COLOR, fontsize=34)
        screen.draw.text(f"Attempts: {attempts}",
                         center=(WIDTH // 2, 300),
                         color=TEXT_COLOR, fontsize=24)
        screen.draw.text("Press SPACE to play again",
                         center=(WIDTH // 2, 330),
                         color=DIM_COLOR, fontsize=18)

# ── Entry point ───────────────────────────────────────────────────────────────
attempts = 1
pgzrun.go()