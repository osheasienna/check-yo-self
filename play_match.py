#!/usr/bin/env python3
"""
Play a simple UCI match between Stockfish and chess-king (via uci_wrapper.py),
WITHOUT modifying uci_wrapper.py.
  - No external dependencies (no python-chess).
  - Uses "position startpos moves ..." to keep both engines in sync.

Examples:
  python3 play_match.py --plies 80 --movetime-ms 100 --stockfish-elo 1600
  python3 play_match.py --plies 40 --movetime-ms 200 --bot-white
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional, TextIO


class UCIEngine:
    def __init__(self, name: str, argv: list[str], cwd: Optional[Path] = None):
        self.name = name
        self.argv = argv
        self.cwd = str(cwd) if cwd else None
        self.proc: Optional[subprocess.Popen[str]] = None

    def start(self) -> None:
        self.proc = subprocess.Popen(
            self.argv,
            cwd=self.cwd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,  # line-buffered
        )
        if not self.proc.stdin or not self.proc.stdout:
            raise RuntimeError(f"{self.name}: failed to open stdin/stdout")

    @property
    def stdin(self) -> TextIO:
        assert self.proc and self.proc.stdin
        return self.proc.stdin

    @property
    def stdout(self) -> TextIO:
        assert self.proc and self.proc.stdout
        return self.proc.stdout

    def send(self, cmd: str) -> None:
        self.stdin.write(cmd + "\n")
        self.stdin.flush()

    def read_line(self) -> str:
        line = self.stdout.readline()
        if line == "":
            raise RuntimeError(f"{self.name}: process exited unexpectedly")
        return line.rstrip("\n")

    def uci_handshake(self) -> None:
        self.send("uci")
        while True:
            line = self.read_line()
            if line == "uciok":
                return

    def isready(self) -> None:
        self.send("isready")
        while True:
            line = self.read_line()
            if line == "readyok":
                return

    def setoption(self, name: str, value: str) -> None:
        self.send(f"setoption name {name} value {value}")

    def newgame(self) -> None:
        self.send("ucinewgame")

    def set_position_startpos(self, moves: list[str]) -> None:
        if moves:
            self.send("position startpos moves " + " ".join(moves))
        else:
            self.send("position startpos")

    def go_movetime(self, movetime_ms: int) -> str:
        self.send(f"go movetime {movetime_ms}")
        best: Optional[str] = None
        while True:
            line = self.read_line()
            if line.startswith("bestmove "):
                parts = line.split()
                best = parts[1] if len(parts) >= 2 else "0000"
                break
        return best or "0000"

    def quit(self) -> None:
        try:
            if self.proc and self.proc.poll() is None:
                self.send("quit")
        finally:
            if self.proc:
                try:
                    self.proc.terminate()
                except Exception:
                    pass


def resolve_stockfish_cmd(explicit: Optional[str]) -> list[str]:
    if explicit:
        return [explicit]
    found = shutil.which("stockfish")
    if not found:
        raise SystemExit(
            "Stockfish introuvable. Installe-le via `brew install stockfish` "
            "ou passe son chemin avec --stockfish-cmd /chemin/vers/stockfish"
        )
    return [found]


# Game result constants
RESULT_BOT_WIN = "chess-king WINS"
RESULT_STOCKFISH_WIN = "stockfish WINS"
RESULT_DRAW = "DRAW"
RESULT_ONGOING = "ONGOING (max plies reached)"


def play_one_game(
    stockfish: UCIEngine,
    bot: UCIEngine,
    plies: int,
    movetime_ms: int,
    bot_plays_white: bool,
    game_num: int,
    total_games: int,
) -> tuple[list[str], str]:
    """Joue une partie et retourne (liste des coups, résultat)."""
    moves: list[str] = []
    result = RESULT_ONGOING
    
    stockfish.newgame()
    bot.newgame()
    stockfish.isready()
    bot.isready()
    
    print(f"\n{'='*60}", flush=True)
    print(f"PARTIE {game_num}/{total_games} - chess-king joue {'les Blancs' if bot_plays_white else 'les Noirs'}", flush=True)
    print(f"{'='*60}\n", flush=True)
    
    for ply in range(plies):
        side_is_white = (ply % 2 == 0)
        engine = bot if (side_is_white == bot_plays_white) else stockfish
        
        stockfish.set_position_startpos(moves)
        bot.set_position_startpos(moves)
        stockfish.isready()
        bot.isready()
        
        best = engine.go_movetime(movetime_ms)
        print(f"ply {ply+1:03d} {engine.name}: {best}", flush=True)
        
        if best in ("0000", "none", "(none)"):
            # Engine returned null move = no legal moves = checkmate or stalemate
            # The engine that couldn't move LOST (or it's stalemate = draw)
            # We assume it's checkmate (most common) - the other side wins
            if engine.name == "stockfish":
                result = RESULT_BOT_WIN
                print(f"\n*** stockfish n'a plus de coups légaux - CHESS-KING GAGNE! ***", flush=True)
            else:
                result = RESULT_STOCKFISH_WIN
                print(f"\n*** chess-king n'a plus de coups légaux - STOCKFISH GAGNE! ***", flush=True)
            break
        
        moves.append(best)
    
    return moves, result


def main() -> int:
    parser = argparse.ArgumentParser(description="Stockfish vs chess-king (uci_wrapper.py)")
    parser.add_argument("--plies", type=int, default=60, help="Nombre de demi-coups (plies) par partie")
    parser.add_argument("--movetime-ms", type=int, default=100, help="Temps par coup en ms (go movetime)")
    parser.add_argument("--bot-white", action="store_true", help="Si présent, chess-king joue toujours les Blancs")
    parser.add_argument("--games", type=int, default=1, help="Nombre de parties à jouer (défaut: 1)")
    parser.add_argument("--alternate-colors", action="store_true", help="Alterner les couleurs entre les parties")

    parser.add_argument("--stockfish-cmd", type=str, default=None, help="Chemin/commande stockfish (optionnel)")
    parser.add_argument("--stockfish-skill", type=int, default=None, help="Skill Level 0..20 (optionnel)")
    parser.add_argument("--stockfish-elo", type=int, default=None, help="Elo (avec UCI_LimitStrength) (optionnel)")

    args = parser.parse_args()

    if args.games < 1:
        raise SystemExit("--games doit être >= 1")

    repo_dir = Path(__file__).resolve().parent
    wrapper_path = repo_dir / "uci_wrapper.py"
    if not wrapper_path.exists():
        raise SystemExit(f"Fichier introuvable: {wrapper_path}")

    # Engines:
    stockfish = UCIEngine("stockfish", resolve_stockfish_cmd(args.stockfish_cmd))
    bot = UCIEngine("chess-king", [sys.executable, str(wrapper_path)], cwd=repo_dir)

    try:
        stockfish.start()
        bot.start()

        stockfish.uci_handshake()
        bot.uci_handshake()

        # Configure Stockfish strength if requested
        if args.stockfish_skill is not None:
            if not (0 <= args.stockfish_skill <= 20):
                raise SystemExit("--stockfish-skill doit être entre 0 et 20")
            stockfish.setoption("Skill Level", str(args.stockfish_skill))

        if args.stockfish_elo is not None:
            stockfish.setoption("UCI_LimitStrength", "true")
            stockfish.setoption("UCI_Elo", str(args.stockfish_elo))

        stockfish.isready()
        bot.isready()

        # Jouer plusieurs parties
        all_games_moves: list[list[str]] = []
        all_games_results: list[str] = []
        bot_wins = 0
        stockfish_wins = 0
        draws = 0
        
        for game_num in range(1, args.games + 1):
            # Déterminer qui joue les blancs pour cette partie
            if args.alternate_colors:
                bot_plays_white = (game_num % 2 == 1)
            else:
                bot_plays_white = bool(args.bot_white)
            
            moves, result = play_one_game(
                stockfish, bot, args.plies, args.movetime_ms,
                bot_plays_white, game_num, args.games
            )
            all_games_moves.append(moves)
            all_games_results.append(result)
            
            # Count results
            if result == RESULT_BOT_WIN:
                bot_wins += 1
            elif result == RESULT_STOCKFISH_WIN:
                stockfish_wins += 1
            else:
                draws += 1
            
            print(f"\nPartie {game_num} terminée - {len(moves)} coups joués", flush=True)
            print(f"Résultat: {result}", flush=True)
            print(f"Moves: {' '.join(moves)}\n", flush=True)

        # Résumé final
        print(f"\n{'='*60}", flush=True)
        print(f"RÉSUMÉ - {args.games} partie(s) jouée(s)", flush=True)
        print(f"{'='*60}", flush=True)
        for i, (moves, result) in enumerate(zip(all_games_moves, all_games_results), 1):
            print(f"Partie {i}: {len(moves)} coups - {result}", flush=True)
        print(f"{'='*60}", flush=True)
        print(f"SCORE FINAL:", flush=True)
        print(f"  chess-king: {bot_wins} victoire(s)", flush=True)
        print(f"  stockfish:  {stockfish_wins} victoire(s)", flush=True)
        print(f"  nulles:     {draws}", flush=True)
        print(f"{'='*60}\n", flush=True)
        
        return 0
    finally:
        bot.quit()
        stockfish.quit()


if __name__ == "__main__":
    raise SystemExit(main())


