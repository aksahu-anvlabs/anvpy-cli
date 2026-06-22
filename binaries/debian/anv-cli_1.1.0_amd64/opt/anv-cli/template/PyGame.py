import pygame

pygame.init()

info = pygame.display.Info()

WIDTH, HEIGHT = info.current_w, info.current_h
screen = pygame.display.set_mode((WIDTH, HEIGHT))
clock = pygame.time.Clock()

# Ball properties
x = WIDTH // 2
y = 100
radius = 25

vx = 3          # Horizontal velocity
vy = 0          # Vertical velocity

gravity = 0.5
bounce = 0.85   # Energy retained after bounce

running = True
while running:
    clock.tick(60)

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_AC_BACK:
                running = False

    # Physics
    vy += gravity

    x += vx
    y += vy

    # Floor collision
    if y + radius >= HEIGHT:
        y = HEIGHT - radius
        vy = -vy * bounce

    # Wall collisions
    if x - radius <= 0:
        x = radius
        vx = -vx

    if x + radius >= WIDTH:
        x = WIDTH - radius
        vx = -vx

    # Stop tiny bouncing
    if abs(vy) < 0.5 and y + radius >= HEIGHT:
        vy = 0

    # Draw
    screen.fill((30, 30, 30))
    pygame.draw.circle(screen, (255, 100, 100), (int(x), int(y)), radius)

    pygame.display.flip()

pygame.quit()
