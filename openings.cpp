#include "openings.h"
#include <unordered_map>

// Opening book: maps move history (comma-separated) to best reply
// Format: "e2e4,e7e5,g1f3" -> next move to play
static const std::unordered_map<std::string, std::string> OPENING_BOOK = {
    // =========================================
    // WHITE OPENINGS (we play first)
    // =========================================
    
    // Starting position: play e4
    {"", "e2e4"},
    
    // After 1.e4 e5 - Italian/Scotch setup
    {"e2e4,e7e5", "g1f3"},                           // 2.Nf3
    {"e2e4,e7e5,g1f3,b8c6", "f1c4"},                 // 3.Bc4 (Italian)
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5", "c2c3"},      // 4.c3 (prep d4)
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6", "d2d3"},      // 4.d3 (solid)
    {"e2e4,e7e5,g1f3,g8f6", "b1c3"},                 // Petrov: 3.Nc3
    
    // After 1.e4 c5 - Open Sicilian
    {"e2e4,c7c5", "g1f3"},                           // 2.Nf3
    {"e2e4,c7c5,g1f3,d7d6", "d2d4"},                 // 3.d4
    {"e2e4,c7c5,g1f3,b8c6", "d2d4"},                 // 3.d4
    {"e2e4,c7c5,g1f3,e7e6", "d2d4"},                 // 3.d4
    
    // After 1.e4 e6 - French Defense
    {"e2e4,e7e6", "d2d4"},                           // 2.d4
    {"e2e4,e7e6,d2d4,d7d5", "b1c3"},                 // 3.Nc3
    
    // After 1.e4 c6 - Caro-Kann
    {"e2e4,c7c6", "d2d4"},                           // 2.d4
    {"e2e4,c7c6,d2d4,d7d5", "b1c3"},                 // 3.Nc3
    
    // After 1.e4 d5 - Scandinavian
    {"e2e4,d7d5", "e4d5"},                           // 2.exd5
    
    // =========================================
    // BLACK OPENINGS (opponent plays first)
    // =========================================
    
    // Against 1.e4 - play e5 (classical)
    {"e2e4", "e7e5"},
    {"e2e4,g1f3", "b8c6"},                           // 2...Nc6
    {"e2e4,f1c4", "g8f6"},                           // 2...Nf6 vs Bishop's Opening
    {"e2e4,d2d4", "e7e5"},                           // vs 2.d4?! play e5
    {"e2e4,b1c3", "g8f6"},                           // vs 2.Nc3 play Nf6
    
    // Against 1.e4 e5 2.Nf3 - continue development
    {"e2e4,e7e5,g1f3", "b8c6"},                      // 2...Nc6
    {"e2e4,e7e5,g1f3,b8c6,f1c4", "f8c5"},           // 3...Bc5 (Italian)
    {"e2e4,e7e5,g1f3,b8c6,f1b5", "a7a6"},           // 3...a6 (Morphy Defense)
    {"e2e4,e7e5,g1f3,b8c6,d2d4", "e5d4"},           // 3...exd4 (Scotch)
    
    // Against 1.d4 - King's Indian setup
    {"d2d4", "g8f6"},                                // 1...Nf6
    {"d2d4,g8f6,c2c4", "g7g6"},                      // 2...g6
    {"d2d4,g8f6,c2c4,g7g6,b1c3", "f8g7"},           // 3...Bg7
    {"d2d4,g8f6,g1f3", "g7g6"},                      // vs 2.Nf3, play g6
    {"d2d4,g8f6,c1f4", "g7g6"},                      // vs London, play g6
    
    // Against 1.c4 - English
    {"c2c4", "e7e5"},                                // 1...e5 (Reversed Sicilian)
    
    // Against 1.Nf3 - flexible
    {"g1f3", "d7d5"},                                // 1...d5
    
    // =========================================
    // COMMON CONTINUATIONS
    // =========================================
    
    // Italian Game continuations
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,c2c3", "g8f6"}, // 4...Nf6
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,c2c3,g8f6,d2d4", "e5d4"}, // 5...exd4
    
    // Sicilian continuations  
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4", "f3d4"},      // 4.Nxd4
    {"e2e4,c7c5,g1f3,b8c6,d2d4,c5d4", "f3d4"},      // 4.Nxd4
    
    // =========================================
    // RUY LOPEZ (SPANISH) - Very Solid
    // =========================================
    {"e2e4,e7e5,g1f3,b8c6,f1b5", "a7a6"},             // 3.Bb5 (Ruy Lopez)
    {"e2e4,e7e5,g1f3,b8c6,f1b5,a7a6", "b5a4"},        // 4.Ba4
    {"e2e4,e7e5,g1f3,b8c6,f1b5,a7a6,b5a4,g8f6", "e1g1"}, // 5.O-O
    {"e2e4,e7e5,g1f3,b8c6,f1b5,a7a6,b5a4,g8f6,e1g1,f8e7", "f1e1"}, // Closed Ruy
    {"e2e4,e7e5,g1f3,b8c6,f1b5,a7a6,b5a4,g8f6,e1g1,b7b5", "a4b3"}, // 6...b5
    {"e2e4,e7e5,g1f3,b8c6,f1b5,g8f6", "e1g1"},        // 4.O-O vs Berlin
    {"e2e4,e7e5,g1f3,b8c6,f1b5,g8f6,e1g1,f6e4", "d2d4"},  // Berlin Defense
    
    // =========================================
    // SCOTCH GAME - Aggressive
    // =========================================
    {"e2e4,e7e5,g1f3,b8c6,d2d4", "e5d4"},             // 3.d4 (Scotch)
    {"e2e4,e7e5,g1f3,b8c6,d2d4,e5d4,f3d4", "f8c5"},   // 4...Bc5
    {"e2e4,e7e5,g1f3,b8c6,d2d4,e5d4,f3d4,g8f6", "d4c6"}, // 5.Nxc6
    
    // =========================================
    // FRENCH DEFENSE - More Lines
    // =========================================
    {"e2e4,e7e6,d2d4,d7d5,e4e5", "c7c5"},             // 3.e5 Advance
    {"e2e4,e7e6,d2d4,d7d5,b1c3,g8f6", "c1g5"},        // 4.Bg5 (Classical)
    {"e2e4,e7e6,d2d4,d7d5,b1c3,f8b4", "e4e5"},        // 4.e5 (Winawer)
    {"e2e4,e7e6,d2d4,d7d5,b1c3,g8f6,c1g5,f8e7", "e4e5"}, // 5.e5
    
    // =========================================
    // CARO-KANN - More Lines
    // =========================================
    {"e2e4,c7c6,d2d4,d7d5,e4e5", "c8f5"},             // 3.e5 Advance
    {"e2e4,c7c6,d2d4,d7d5,b1c3,d5e4", "c3e4"},        // 4.Nxe4
    {"e2e4,c7c6,d2d4,d7d5,b1c3,d5e4,c3e4,b8d7", "g1f3"}, // 5.Nf3
    {"e2e4,c7c6,d2d4,d7d5,b1c3,d5e4,c3e4,c8f5", "e4g3"}, // 5.Ng3
    
    // =========================================
    // SICILIAN - Najdorf, Dragon, Kan
    // =========================================
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6", "b1c3"}, // 5.Nc3
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3,a7a6", "c1e3"}, // 6.Be3 Najdorf
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3,a7a6,c1e3,e7e5", "d4b3"}, // 7.Nb3
    {"e2e4,c7c5,g1f3,b8c6,d2d4,c5d4,f3d4,g7g6", "b1c3"}, // Dragon setup
    {"e2e4,c7c5,g1f3,b8c6,d2d4,c5d4,f3d4,g7g6,b1c3,f8g7", "c1e3"}, // Dragon 6.Be3
    {"e2e4,c7c5,g1f3,e7e6,d2d4,c5d4,f3d4,a7a6", "b1c3"}, // Kan/Taimanov
    
    // =========================================
    // SCANDINAVIAN - Punish It
    // =========================================
    {"e2e4,d7d5,e4d5,d8d5", "b1c3"},                  // 3.Nc3 (attack queen)
    {"e2e4,d7d5,e4d5,d8d5,b1c3,d5a5", "d2d4"},        // 4.d4
    {"e2e4,d7d5,e4d5,g8f6", "d2d4"},                  // 3.d4 (vs Nf6)
    {"e2e4,d7d5,e4d5,d8d5,b1c3,d5a5,d2d4,g8f6", "g1f3"}, // 5.Nf3
    
    // =========================================
    // ALEKHINE'S DEFENSE
    // =========================================
    {"e2e4,g8f6", "e4e5"},                            // 2.e5 (chase knight)
    {"e2e4,g8f6,e4e5,f6d5", "d2d4"},                  // 3.d4
    {"e2e4,g8f6,e4e5,f6d5,d2d4,d7d6", "g1f3"},        // 4.Nf3
    
    // =========================================
    // PIRC/MODERN DEFENSE
    // =========================================
    {"e2e4,d7d6", "d2d4"},                            // 2.d4
    {"e2e4,d7d6,d2d4,g8f6", "b1c3"},                  // 3.Nc3
    {"e2e4,d7d6,d2d4,g8f6,b1c3,g7g6", "f1c4"},        // 4.Bc4
    {"e2e4,g7g6", "d2d4"},                            // vs Modern: 2.d4
    {"e2e4,g7g6,d2d4,f8g7", "b1c3"},                  // 3.Nc3
    
    // =========================================
    // BLACK: QUEEN'S GAMBIT RESPONSES
    // =========================================
    {"d2d4,d7d5,c2c4", "e7e6"},                       // QGD
    {"d2d4,d7d5,c2c4,e7e6", "g8f6"},                  // Develop
    {"d2d4,d7d5,c2c4,e7e6,b1c3", "g8f6"},             // 3...Nf6
    {"d2d4,d7d5,c2c4,c7c6", "g8f6"},                  // Slav Defense
    {"d2d4,d7d5,c2c4,c7c6,g1f3", "g8f6"},             // Slav main
    {"d2d4,d7d5,c2c4,d5c4", "g1f3"},                  // QGA
    
    // =========================================
    // BLACK: KING'S INDIAN - Deeper Lines
    // =========================================
    {"d2d4,g8f6,c2c4,g7g6,b1c3,f8g7,e2e4", "d7d6"},  // Main line
    {"d2d4,g8f6,c2c4,g7g6,b1c3,f8g7,e2e4,d7d6,g1f3", "e8g8"}, // Castle
    {"d2d4,g8f6,c2c4,g7g6,b1c3,f8g7,e2e4,d7d6,g1f3,e8g8,f1e2", "e7e5"}, // Main line
    {"d2d4,g8f6,c2c4,g7g6,g1f3,f8g7", "b1c3"},        // Fianchetto line
    
    // =========================================
    // BLACK: VS LONDON SYSTEM
    // =========================================
    {"d2d4,g8f6,c1f4", "d7d5"},                       // Solid response
    {"d2d4,g8f6,c1f4,d7d5,e2e3", "e7e6"},             // Continue
    {"d2d4,g8f6,c1f4,d7d5,e2e3,e7e6,g1f3", "f8d6"},   // Challenge bishop
    {"d2d4,d7d5,c1f4", "g8f6"},                       // Transpose
    {"d2d4,d7d5,c1f4,g8f6,e2e3", "e7e6"},             // Solid
    
    // =========================================
    // BLACK: VS ENGLISH (1.c4)
    // =========================================
    {"c2c4,e7e5,b1c3", "g8f6"},                       // Develop
    {"c2c4,e7e5,b1c3,g8f6,g1f3", "b8c6"},             // Continue
    {"c2c4,e7e5,b1c3,b8c6", "g1f3"},                  // 3.Nf3
    {"c2c4,g8f6", "g7g6"},                            // KID setup
    {"c2c4,c7c5", "g1f3"},                            // Symmetrical
    
    // =========================================
    // BLACK: VS 1.Nf3 SYSTEMS
    // =========================================
    {"g1f3,d7d5,d2d4", "g8f6"},                       // Transpose to d4
    {"g1f3,g8f6", "d7d5"},                            // Flexible
    {"g1f3,d7d5,g2g3", "g8f6"},                       // vs Catalan-like
    
    // =========================================
    // BLACK: VS RARE OPENINGS
    // =========================================
    {"b2b3", "e7e5"},                                 // Grab center
    {"b2b3,e7e5", "d7d5"},                            // Control center
    {"g2g3", "d7d5"},                                 // Grab center
    {"g2g3,d7d5,f1g2", "g8f6"},                       // Develop
    {"f2f4", "d7d5"},                                 // vs Bird's
    
    // =========================================
    // BLACK: SICILIAN DEFENSE (as Black vs 1.e4)
    // =========================================
    // Alternative to 1...e5, play Sicilian
    // {"e2e4", "c7c5"},  // Uncomment to play Sicilian as Black
    
    // If transposing to Sicilian positions
    {"e2e4,c7c5,g1f3", "d7d6"},                       // 2...d6 (Najdorf setup)
    {"e2e4,c7c5,g1f3,d7d6,d2d4", "c5d4"},            // 3...cxd4
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4", "g8f6"}, // 4...Nf6
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3", "a7a6"}, // Najdorf!
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3,a7a6,c1e3", "e7e5"}, // Main Najdorf
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3,a7a6,f1e2", "e7e5"}, // English Attack
    {"e2e4,c7c5,g1f3,b8c6", "d7d6"},                  // 2...Nc6, then d6
    {"e2e4,c7c5,g1f3,b8c6,d2d4,c5d4,f3d4", "g8f6"},  // 4...Nf6
    {"e2e4,c7c5,g1f3,e7e6", "b8c6"},                  // Taimanov/Kan
    {"e2e4,c7c5,g1f3,e7e6,d2d4,c5d4,f3d4", "a7a6"},  // Kan main line
    
    // Dragon variation
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3,g7g6", "c1e3"}, // Dragon
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3,g7g6,c1e3", "f8g7"}, // Dragon Bg7
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4,f3d4,g8f6,b1c3,g7g6,c1e3,f8g7,f2f3", "e8g8"}, // Yugoslav
    
    // =========================================
    // BLACK: NIMZO-INDIAN DEFENSE
    // =========================================
    {"d2d4,g8f6,c2c4,e7e6", "f8b4"},                  // 3...Bb4 (Nimzo-Indian)
    {"d2d4,g8f6,c2c4,e7e6,b1c3", "f8b4"},             // Nimzo-Indian main
    {"d2d4,g8f6,c2c4,e7e6,b1c3,f8b4,d1c2", "e8g8"},  // 4.Qc2: Castle
    {"d2d4,g8f6,c2c4,e7e6,b1c3,f8b4,e2e3", "e8g8"},  // 4.e3: Castle
    {"d2d4,g8f6,c2c4,e7e6,b1c3,f8b4,e2e3,e8g8,f1d3", "d7d5"}, // Classical
    {"d2d4,g8f6,c2c4,e7e6,b1c3,f8b4,a2a3", "b4c3"},  // Samisch: take
    {"d2d4,g8f6,c2c4,e7e6,b1c3,f8b4,a2a3,b4c3,b2c3", "c7c5"}, // Samisch c5
    
    // =========================================
    // BLACK: QUEEN'S INDIAN DEFENSE
    // =========================================
    {"d2d4,g8f6,c2c4,e7e6,g1f3", "b7b6"},             // Queen's Indian
    {"d2d4,g8f6,c2c4,e7e6,g1f3,b7b6,g2g3", "c8b7"},  // Fianchetto
    {"d2d4,g8f6,c2c4,e7e6,g1f3,b7b6,g2g3,c8b7,f1g2", "f8e7"}, // Main line
    {"d2d4,g8f6,c2c4,e7e6,g1f3,b7b6,a2a3", "c8b7"},  // Petrosian
    {"d2d4,g8f6,c2c4,e7e6,g1f3,b7b6,b1c3", "c8b7"},  // 4.Nc3
    
    // =========================================
    // BLACK: GRUNFELD DEFENSE
    // =========================================
    {"d2d4,g8f6,c2c4,g7g6,b1c3,d7d5", "c4d5"},       // Grunfeld main
    {"d2d4,g8f6,c2c4,g7g6,b1c3,d7d5,c4d5", "f6d5"},  // Nxd5
    {"d2d4,g8f6,c2c4,g7g6,b1c3,d7d5,c4d5,f6d5,e2e4", "d5c3"}, // Exchange
    {"d2d4,g8f6,c2c4,g7g6,b1c3,d7d5,c4d5,f6d5,e2e4,d5c3,b2c3", "f8g7"}, // Main
    {"d2d4,g8f6,c2c4,g7g6,b1c3,d7d5,g1f3", "f8g7"},  // Russian system
    
    // =========================================
    // BLACK: DUTCH DEFENSE  
    // =========================================
    {"d2d4,f7f5", "g1f3"},                            // Dutch
    {"d2d4,f7f5,g1f3", "g8f6"},                       // Classical Dutch
    {"d2d4,f7f5,g1f3,g8f6,g2g3", "e7e6"},            // Stonewall setup
    {"d2d4,f7f5,g1f3,g8f6,g2g3,e7e6,f1g2", "f8e7"}, // Classical
    {"d2d4,f7f5,c2c4", "g8f6"},                       // Leningrad setup
    {"d2d4,f7f5,c2c4,g8f6,g2g3", "g7g6"},            // Leningrad Dutch
    
    // =========================================
    // BLACK: BENONI DEFENSE
    // =========================================
    {"d2d4,g8f6,c2c4,c7c5", "d4d5"},                  // Benoni
    {"d2d4,g8f6,c2c4,c7c5,d4d5,e7e6", "b1c3"},       // Modern Benoni
    {"d2d4,g8f6,c2c4,c7c5,d4d5,e7e6,b1c3,e6d5", "c4d5"}, // Main Benoni
    {"d2d4,g8f6,c2c4,c7c5,d4d5,e7e6,b1c3,e6d5,c4d5,d7d6", "e2e4"}, // Benoni main
    
    // =========================================
    // BLACK: BOGO-INDIAN DEFENSE
    // =========================================
    {"d2d4,g8f6,c2c4,e7e6,g1f3,f8b4", "c1d2"},       // Bogo-Indian
    {"d2d4,g8f6,c2c4,e7e6,g1f3,f8b4,c1d2", "b4d2"},  // Exchange
    {"d2d4,g8f6,c2c4,e7e6,g1f3,f8b4,c1d2,b4d2,d1d2", "e8g8"}, // Castle
    {"d2d4,g8f6,c2c4,e7e6,g1f3,f8b4,b1d2", "b7b6"},  // vs Nbd2
    
    // =========================================
    // BLACK: CATALAN RESPONSES
    // =========================================
    {"d2d4,g8f6,c2c4,e7e6,g2g3", "d7d5"},            // vs Catalan
    {"d2d4,g8f6,c2c4,e7e6,g2g3,d7d5,f1g2", "f8e7"}, // Closed Catalan
    {"d2d4,g8f6,c2c4,e7e6,g2g3,d7d5,f1g2,d5c4", "d1a4"}, // Open Catalan
    {"d2d4,g8f6,c2c4,e7e6,g2g3,d7d5,f1g2,f8e7,g1f3", "e8g8"}, // Castle
    
    // =========================================
    // BLACK: DEEPER e5 RESPONSES
    // =========================================
    // After 1.e4 e5 - more continuations
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6", "d2d3"},       // Two Knights
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6,d2d3", "f8e7"}, // Solid
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6,d2d3,f8e7,e1g1", "e8g8"}, // Both castle
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6,f3g5", "d7d5"}, // Fried Liver defense
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6,f3g5,d7d5,e4d5", "b8a5"}, // Na5!
    
    // Four Knights
    {"e2e4,e7e5,g1f3,b8c6,b1c3", "g8f6"},            // Four Knights
    {"e2e4,e7e5,g1f3,b8c6,b1c3,g8f6,f1b5", "f8b4"}, // Symmetrical
    {"e2e4,e7e5,g1f3,b8c6,b1c3,g8f6,d2d4", "e5d4"}, // Scotch Four Knights
    
    // King's Gambit declined
    {"e2e4,e7e5,f2f4", "f8c5"},                       // King's Gambit Declined
    {"e2e4,e7e5,f2f4,f8c5,g1f3", "d7d6"},            // Solid
    
    // Vienna Game
    {"e2e4,e7e5,b1c3", "g8f6"},                       // Vienna: Nf6
    {"e2e4,e7e5,b1c3,g8f6,f2f4", "d7d5"},            // Vienna Gambit: d5!
    {"e2e4,e7e5,b1c3,b8c6", "f1c4"},                  // Vienna: Nc6
    
    // Bishop's Opening
    {"e2e4,e7e5,f1c4", "g8f6"},                       // Attack e4
    {"e2e4,e7e5,f1c4,g8f6,d2d3", "f8c5"},            // Develop
    
    // =========================================
    // BLACK: PETROFF DEFENSE (1.e4 e5 2.Nf3 Nf6)
    // =========================================
    // Alternative Black response to 2.Nf3
    {"e2e4,e7e5,g1f3,g8f6", "f3e5"},                  // Main line
    {"e2e4,e7e5,g1f3,g8f6,f3e5,d7d6", "e5f3"},       // 3...d6
    {"e2e4,e7e5,g1f3,g8f6,f3e5,d7d6,e5f3,f6e4", "d2d4"}, // Classical
    {"e2e4,e7e5,g1f3,g8f6,b1c3", "b8c6"},            // Three Knights
    {"e2e4,e7e5,g1f3,g8f6,d2d4", "f6e4"},            // Steinitz
    
    // =========================================
    // DEEP ITALIAN LINES
    // =========================================
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,c2c3,g8f6,d2d4,e5d4,c3d4", "c5b4"}, // 6...Bb4+
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,d2d3", "g8f6"},   // Giuoco Pianissimo
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,d2d3,g8f6,c2c3", "d7d6"}, // Slow Italian
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,d2d3,g8f6,e1g1", "d7d6"}, // Castled Italian
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6,d2d3,f8e7", "e1g1"}, // Two Knights
    
    // =========================================
    // CASTLING PRIORITIZATION
    // =========================================
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,c2c3,g8f6,d2d3", "e1g1"}, // White O-O
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,e1g1", "g8f6"},   // After White castles
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6,d2d3,f8e7,e1g1", "e8g8"}, // Black O-O
    {"d2d4,g8f6,c2c4,g7g6,b1c3,f8g7,e2e4,d7d6,f1e2", "e8g8"}, // KID O-O
};

// Helper: convert move history vector to lookup key
static std::string history_to_key(const std::vector<std::string>& move_history) {
    std::string key;
    for (size_t i = 0; i < move_history.size(); ++i) {
        if (i > 0) key += ",";
        key += move_history[i];
    }
    return key;
}

// Helper: parse UCI move string to move struct
static move parse_uci_move(const std::string& uci) {
    if (uci.length() < 4) {
        return move(0, 0, 0, 0);  // Invalid
    }
    
    int from_col = uci[0] - 'a';
    int from_row = uci[1] - '1';
    int to_col = uci[2] - 'a';
    int to_row = uci[3] - '1';
    
    promotion_piece_type promo = NONE;
    if (uci.length() >= 5) {
        switch (uci[4]) {
            case 'q': promo = QUEEN; break;
            case 'r': promo = ROOK; break;
            case 'b': promo = BISHOP; break;
            case 'n': promo = KNIGHT; break;
        }
    }
    
    return move(from_row, from_col, to_row, to_col, promo);
}

move get_book_move(const std::vector<std::string>& move_history) {
    // Don't use book after move 12 (24 half-moves)
    if (move_history.size() > 24) {
        return move(0, 0, 0, 0);
    }
    
    std::string key = history_to_key(move_history);
    
    auto it = OPENING_BOOK.find(key);
    if (it != OPENING_BOOK.end()) {
        return parse_uci_move(it->second);
    }
    
    // Not in book
    return move(0, 0, 0, 0);
}

bool is_opening_phase(int move_count) {
    // Opening is roughly the first 10-12 moves (20-24 half-moves)
    return move_count < 20;
}