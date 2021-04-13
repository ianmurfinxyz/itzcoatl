
<p align="center">
  <img src="img/itzcoatl_title.png" alt="itzcoatl_title"/>
</p>

## What's this?

Itcoatl is an Aztec themed snake game made to test drive my engine Pixiretro. It features a main menu and a high scorer's leaderboard as well as the game itself. The gameplay consists of eating as many nuggets as you can as quickly as you can, gaining score bonuses for speed and nugget combos, and has 8 playable snakes each named after an Aztec emperor.

## Gameplay

As is standard with snake games you play as a snake which must eat as many nuggets as possible whilst growing in length with each nugget eaten. In this version the snake does not collide with the world edges opting instead to wrap around to the other side as shown in the screenshot below. The only obstacle in the game is thus yourself with a single bite of your own body resulting in immediete game over.

<p align="center">
  <img src="img/game_shot.png" alt="gameplay_screenshot"/>
</p>

There are 7 varieties of nugget to eat in the game with each nugget having a unique score value as shown in the table below. Scores can be multiplied by eating nuggets in quick succession or by eating x3 of a single nugget type in sequence. The quick succession bonus increases with longer chains of quick eats. Eating a single nugget and then a second nugget before the quick bonus cooldown will boost the quick bonus to +10% giving an extra 10% to all future nuggets eaten. Continuing to eat nuggets before the quick cooldown will rack up the quick bonus up to a max of 250%. However if the cooldown drops to zero you lose all accumulated quick bonus and have to build it back up.

The two types of bonuses can be combined too, so if you have a current quick bonus of +250% and then eat x3 of a single nugget type in sequence the +250% applies to the combo bonus too.

<p align="center">
  <img src="img/nuggets.png" alt="nugget_value_table"/>
</p>

## Controls

## Compilation

## Credits