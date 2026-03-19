import * as Clipboard from 'expo-clipboard'
import { useLocalSearchParams, useRouter } from 'expo-router'
import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import { Alert, StyleSheet, Text, TouchableOpacity, View } from 'react-native'

import ChessBoard from '@/components/ChessBoard'
import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'
import { ChessEngine, Color } from '@/lib/chess'

export default function GameScreen() {
  const { settings } = useSettings()
  const params = useLocalSearchParams()
  const router = useRouter()

  // Parse new per-player parameters
  const whitePlayerType = (params.whitePlayerType as string) || 'human'
  const blackPlayerType = (params.blackPlayerType as string) || 'ai'

  const whiteMaxAITime = parseInt(params.whiteMaxAITime as string, 10) || 5
  const whiteMaxDepth = parseInt(params.whiteMaxDepth as string, 10) || 5
  const whiteAIVersion = (params.whiteAIVersion as 'v1' | 'v2') || settings.aiVersion
  const blackMaxAITime = parseInt(params.blackMaxAITime as string, 10) || 5
  const blackMaxDepth = parseInt(params.blackMaxDepth as string, 10) || 5
  const blackAIVersion = (params.blackAIVersion as 'v1' | 'v2') || settings.aiVersion

  const useChessClock = params.useChessClock === 'true'
  const clockTimeMinutes = parseInt(params.clockTimeMinutes as string, 10) || 10

  const [currentPlayer, setCurrentPlayer] = useState<Color>(Color.WHITE)
  const [whiteTimeMs, setWhiteTimeMs] = useState(clockTimeMinutes * 60 * 1000)
  const [blackTimeMs, setBlackTimeMs] = useState(clockTimeMinutes * 60 * 1000)
  const [clockStarted, setClockStarted] = useState(false)
  const [gameOver, setGameOver] = useState(false)
  const [moveCount, setMoveCount] = useState(0)
  const timerRef = useRef<NodeJS.Timeout | null>(null)
  const engineRef = useRef<ChessEngine | null>(null)

  const handleEngineReady = useCallback((engine: ChessEngine) => {
    engineRef.current = engine
  }, [])

  const getResultToken = useCallback((engine: ChessEngine): string => {
    if (engine.isCheckmate()) {
      return engine.getCurrentPlayer() === Color.WHITE ? '0-1' : '1-0'
    }

    if (engine.isStalemate() || engine.isDraw()) {
      return '1/2-1/2'
    }

    // Product requirement: treat in-progress games as draws when exporting PGN.
    return '1/2-1/2'
  }, [])

  const buildPgn = useCallback(
    (engine: ChessEngine): string => {
      const result = getResultToken(engine)
      const moveHistory = engine.getMoveHistory()
      const date = new Date()
      const yyyy = date.getFullYear()
      const mm = `${date.getMonth() + 1}`.padStart(2, '0')
      const dd = `${date.getDate()}`.padStart(2, '0')

      const whiteLabel = whitePlayerType === 'ai' ? `AI (${whiteAIVersion})` : 'Human'
      const blackLabel = blackPlayerType === 'ai' ? `AI (${blackAIVersion})` : 'Human'

      const headers = [
        '[Event "Chess App Casual Game"]',
        '[Site "Local App"]',
        `[Date "${yyyy}.${mm}.${dd}"]`,
        '[Round "-"]',
        `[White "${whiteLabel}"]`,
        `[Black "${blackLabel}"]`,
        `[Result "${result}"]`,
      ]

      const formatMoveForPgn = (move: string): string => {
        const promotionMatch = move.match(/^([a-h][1-8][a-h][1-8])([qrbn])$/i)
        if (!promotionMatch) {
          return move
        }

        return `${promotionMatch[1]}=${promotionMatch[2].toUpperCase()}`
      }

      const moveTextParts: string[] = []
      for (let i = 0; i < moveHistory.length; i += 2) {
        const whiteMove = formatMoveForPgn(moveHistory[i])
        const blackMove = moveHistory[i + 1] ? formatMoveForPgn(moveHistory[i + 1]) : undefined
        const fullMove = Math.floor(i / 2) + 1
        if (blackMove) {
          moveTextParts.push(`${fullMove}. ${whiteMove} ${blackMove}`)
        } else {
          moveTextParts.push(`${fullMove}. ${whiteMove}`)
        }
      }

      const moveText = `${moveTextParts.join(' ')} ${result}`.trim()
      return `${headers.join('\n')}\n\n${moveText}`
    },
    [blackAIVersion, blackPlayerType, getResultToken, whiteAIVersion, whitePlayerType],
  )

  const handleCopyPgn = useCallback(async () => {
    const engine = engineRef.current

    if (!engine) {
      Alert.alert('PGN Unavailable', 'Game state is still initializing. Please try again.')
      return
    }

    try {
      const pgn = buildPgn(engine)
      await Clipboard.setStringAsync(pgn)
      Alert.alert('PGN Copied', 'Game PGN copied to clipboard.')
    } catch (error) {
      console.error('GameScreen: Failed to copy PGN:', error)
      Alert.alert('Copy Failed', 'Could not copy PGN. Please try again.')
    }
  }, [buildPgn])

  const handleCurrentPlayerChange = useCallback(
    (player: Color) => {
      setCurrentPlayer(player)
      setMoveCount((prev) => prev + 1)

      // Start clock after White's first move (after move 1, it's Black's turn)
      if (useChessClock && !clockStarted && moveCount === 0) {
        setClockStarted(true)
      }
    },
    [useChessClock, clockStarted, moveCount],
  )

  // Clock countdown effect
  useEffect(() => {
    if (!useChessClock || !clockStarted || gameOver) {
      return
    }

    timerRef.current = setInterval(() => {
      if (currentPlayer === Color.WHITE) {
        setWhiteTimeMs((prev) => {
          const newTime = Math.max(0, prev - 100)
          if (newTime === 0) {
            setGameOver(true)
          }
          return newTime
        })
      } else {
        setBlackTimeMs((prev) => {
          const newTime = Math.max(0, prev - 100)
          if (newTime === 0) {
            setGameOver(true)
          }
          return newTime
        })
      }
    }, 100)

    return () => {
      if (timerRef.current) {
        clearInterval(timerRef.current)
      }
    }
  }, [useChessClock, clockStarted, currentPlayer, gameOver])

  const formatTime = (ms: number): string => {
    const totalSeconds = Math.floor(ms / 1000)
    const minutes = Math.floor(totalSeconds / 60)
    const seconds = totalSeconds % 60
    return `${minutes}:${seconds.toString().padStart(2, '0')}`
  }

  // Check if current player is AI
  const isAITurn = useMemo(() => {
    if (currentPlayer === Color.WHITE) {
      return whitePlayerType === 'ai'
    } else {
      return blackPlayerType === 'ai'
    }
  }, [whitePlayerType, blackPlayerType, currentPlayer])

  // Get AI settings for current player
  const currentMaxAITime = currentPlayer === Color.WHITE ? whiteMaxAITime : blackMaxAITime
  const currentMaxDepth = currentPlayer === Color.WHITE ? whiteMaxDepth : blackMaxDepth
  const currentAIVersion = currentPlayer === Color.WHITE ? whiteAIVersion : blackAIVersion

  return (
    <View style={styles.container}>
      <View style={styles.headerBar}>
        <TouchableOpacity style={styles.backButton} onPress={() => router.back()}>
          <Text style={styles.backButtonText}>← Back</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.copyPgnButton} onPress={handleCopyPgn}>
          <Text style={styles.copyPgnButtonText}>Copy PGN</Text>
        </TouchableOpacity>
      </View>

      {useChessClock && (
        <View style={styles.clockContainer}>
          <View
            style={[
              styles.clockItem,
              currentPlayer === Color.BLACK && clockStarted && styles.activeClockBlack,
            ]}
          >
            <Text style={styles.clockLabel}>⚫ Black</Text>
            <Text style={[styles.clockTime, blackTimeMs === 0 && styles.clockTimeOut]}>
              {formatTime(blackTimeMs)}
            </Text>
          </View>

          <View
            style={[
              styles.clockItem,
              currentPlayer === Color.WHITE && clockStarted && styles.activeClockWhite,
            ]}
          >
            <Text style={styles.clockLabel}>⚪ White</Text>
            <Text style={[styles.clockTime, whiteTimeMs === 0 && styles.clockTimeOut]}>
              {formatTime(whiteTimeMs)}
            </Text>
          </View>
        </View>
      )}

      {gameOver && (
        <View style={styles.gameOverBanner}>
          <Text style={styles.gameOverText}>
            Time Out! {whiteTimeMs === 0 ? '⚫ Black' : '⚪ White'} wins!
          </Text>
        </View>
      )}

      <ChessBoard
        autoFlip={settings.autoFlipBoard}
        searchDepth={currentMaxDepth}
        maxSearchTime={currentMaxAITime * 1000}
        isAIEnabled={isAITurn && !gameOver}
        aiVersion={currentAIVersion}
        onCurrentPlayerChange={handleCurrentPlayerChange}
        onEngineReady={handleEngineReady}
        hideBestMove
        useChessClock={useChessClock}
        clockTimeMinutes={clockTimeMinutes}
      />
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  headerBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 12,
    paddingTop: 24,
    paddingBottom: 6,
    backgroundColor: theme.colors.background.light,
  },
  backButton: {
    alignSelf: 'flex-start',
    backgroundColor: theme.colors.primary,
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
  },
  backButtonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  copyPgnButton: {
    alignSelf: 'flex-start',
    backgroundColor: '#1f2937',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
  },
  copyPgnButtonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  clockContainer: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    alignItems: 'center',
    paddingHorizontal: 12,
    paddingTop: 4,
    paddingBottom: 4,
    backgroundColor: theme.colors.background.light,
    gap: 8,
  },
  clockItem: {
    flex: 1,
    backgroundColor: '#f0f0f0',
    borderRadius: 8,
    padding: 8,
    alignItems: 'center',
    borderWidth: 2,
    borderColor: 'transparent',
  },
  activeClockWhite: {
    borderColor: theme.colors.primary,
    backgroundColor: '#fff',
  },
  activeClockBlack: {
    borderColor: theme.colors.primary,
    backgroundColor: '#fff',
  },
  clockLabel: {
    fontSize: 13,
    fontWeight: '600',
    marginBottom: 4,
    color: theme.colors.text.light,
  },
  clockTime: {
    fontSize: 22,
    fontWeight: 'bold',
    color: theme.colors.text.light,
    fontVariant: ['tabular-nums'],
  },
  clockTimeOut: {
    color: theme.colors.error,
  },
  gameOverBanner: {
    backgroundColor: theme.colors.error,
    paddingVertical: 6,
    paddingHorizontal: 12,
    alignItems: 'center',
  },
  gameOverText: {
    color: '#fff',
    fontSize: 15,
    fontWeight: 'bold',
  },
})
