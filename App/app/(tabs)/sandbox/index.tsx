import Slider from '@react-native-community/slider'
import { useCallback, useRef, useState } from 'react'
import { StyleSheet, Text, TouchableOpacity, View } from 'react-native'

import ChessBoard, { type ChessBoardRef } from '@/components/ChessBoard'
import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'
import type { ChessEngine } from '@/lib/chess'

function EvaluationBar({ score, depth }: { score: number; depth: number }) {
  // Convert centipawns to pawns (divide by 100)
  // Clamp between -10 and 10
  const pawns = Math.max(-10, Math.min(10, score / 100))

  // Calculate percentage for visual bar (0-100)
  // -10 pawns = 0%, 0 pawns = 50%, +10 pawns = 100%
  const percentage = ((pawns + 10) / 20) * 100

  return (
    <View style={evaluationStyles.container}>
      <View style={evaluationStyles.barContainer}>
        <View style={evaluationStyles.blackSection} />
        <View style={[evaluationStyles.whiteSection, { width: `${percentage}%` }]} />
      </View>
      <View style={evaluationStyles.infoRow}>
        <Text style={evaluationStyles.scoreText}>
          {pawns >= 0 ? '+' : ''}
          {pawns.toFixed(1)}
        </Text>
        {depth > 0 && <Text style={evaluationStyles.depthText}>Depth: {depth}</Text>}
      </View>
    </View>
  )
}

const evaluationStyles = StyleSheet.create({
  container: {
    width: '100%',
    paddingHorizontal: 12,
    paddingTop: 4,
    paddingBottom: 6,
  },
  barContainer: {
    height: 20,
    width: '100%',
    backgroundColor: '#000',
    borderRadius: 4,
    overflow: 'hidden',
    position: 'relative',
  },
  blackSection: {
    position: 'absolute',
    left: 0,
    top: 0,
    bottom: 0,
    right: 0,
    backgroundColor: '#000',
  },
  whiteSection: {
    position: 'absolute',
    left: 0,
    top: 0,
    bottom: 0,
    backgroundColor: '#fff',
  },
  scoreText: {
    fontSize: 13,
    fontWeight: '600',
    color: theme.colors.text.light,
    textAlign: 'center',
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    marginTop: 2,
    gap: 8,
  },
  depthText: {
    fontSize: 11,
    fontWeight: '500',
    color: theme.colors.text.light,
    opacity: 0.7,
  },
})

export default function SandboxScreen() {
  const { settings } = useSettings()
  const [searchDepth, setSearchDepth] = useState(3)
  const [evaluation, setEvaluation] = useState(0)
  const [evalDepth, setEvalDepth] = useState(0)
  const [moveHistory, setMoveHistory] = useState<string[]>([])
  const [historyIndex, setHistoryIndex] = useState(-1)
  const engineRef = useRef<ChessEngine | null>(null)
  const chessBoardRef = useRef<ChessBoardRef>(null)

  const handleEngineReady = useCallback((engine: ChessEngine) => {
    engineRef.current = engine
  }, [])

  const handleEvaluationChange = useCallback(
    (score: number, depth: number) => {
      setEvaluation(score)
      setEvalDepth(depth)
      // Update move history whenever evaluation changes (means a move was made)
      if (engineRef.current) {
        const history = engineRef.current.getMoveHistory()
        const newHistoryLength = history.length
        const prevHistoryLength = moveHistory.length

        // If a new move was made (history got longer)
        if (newHistoryLength > prevHistoryLength) {
          // If we were in the middle of history, this truncates the "future" moves
          setMoveHistory(history)
          setHistoryIndex(newHistoryLength - 1)
        }
      }
    },
    [moveHistory.length],
  )

  const handleReset = useCallback(() => {
    if (chessBoardRef.current) {
      chessBoardRef.current.resetGame()
      setMoveHistory([])
      setHistoryIndex(-1)
      setEvaluation(0)
      setEvalDepth(0)
    }
  }, [])

  const handlePreviousMove = useCallback(() => {
    if (engineRef.current && chessBoardRef.current && historyIndex >= 0) {
      engineRef.current.undoMove()
      setHistoryIndex(historyIndex - 1)
      chessBoardRef.current.refresh()
    }
  }, [historyIndex])

  const handleNextMove = useCallback(() => {
    if (engineRef.current && chessBoardRef.current && historyIndex < moveHistory.length - 1) {
      const move = moveHistory[historyIndex + 1]
      engineRef.current.makeMove(move)
      setHistoryIndex(historyIndex + 1)
      chessBoardRef.current.refresh()
    }
  }, [historyIndex, moveHistory])

  return (
    <View style={styles.container}>
      <EvaluationBar score={evaluation} depth={evalDepth} />

      <View style={styles.controls}>
        <View style={styles.controlRow}>
          <Text style={styles.label}>Search Depth: {searchDepth}</Text>
          <Slider
            style={styles.slider}
            minimumValue={1}
            maximumValue={10}
            step={1}
            value={searchDepth}
            onValueChange={setSearchDepth}
            minimumTrackTintColor={theme.colors.primary}
            maximumTrackTintColor={theme.colors.background.dark}
            thumbTintColor={theme.colors.primary}
          />
        </View>
      </View>

      <ChessBoard
        ref={chessBoardRef}
        autoFlip={settings.autoFlipBoard}
        searchDepth={searchDepth}
        onEvaluationChange={handleEvaluationChange}
        onEngineReady={handleEngineReady}
        showControls={false}
      />

      <View style={styles.bottomControls}>
        <View style={styles.navigationRow}>
          <TouchableOpacity
            style={[styles.navButton, historyIndex < 0 && styles.navButtonDisabled]}
            onPress={handlePreviousMove}
            disabled={historyIndex < 0}
          >
            <Text style={styles.navButtonText}>← Back</Text>
          </TouchableOpacity>

          <TouchableOpacity style={styles.resetButton} onPress={handleReset}>
            <Text style={styles.resetButtonText}>Reset</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[
              styles.navButton,
              historyIndex >= moveHistory.length - 1 && styles.navButtonDisabled,
            ]}
            onPress={handleNextMove}
            disabled={historyIndex >= moveHistory.length - 1}
          >
            <Text style={styles.navButtonText}>Forward →</Text>
          </TouchableOpacity>
        </View>
      </View>
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
  },
  controls: {
    paddingHorizontal: 12,
    paddingBottom: 4,
    gap: 4,
  },
  controlRow: {
    gap: 2,
  },
  label: {
    fontSize: 14,
    fontWeight: '600',
    color: theme.colors.text.light,
  },
  slider: {
    width: '100%',
    height: 28,
  },
  bottomControls: {
    paddingHorizontal: 12,
    paddingVertical: 8,
    backgroundColor: theme.colors.background.light,
  },
  navigationRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    gap: 12,
  },
  navButton: {
    flex: 1,
    backgroundColor: theme.colors.primary,
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  navButtonDisabled: {
    backgroundColor: '#ccc',
    opacity: 0.5,
  },
  navButtonText: {
    color: '#fff',
    fontSize: 15,
    fontWeight: '600',
  },
  resetButton: {
    flex: 1,
    backgroundColor: theme.colors.error,
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  resetButtonText: {
    color: '#fff',
    fontSize: 15,
    fontWeight: '600',
  },
})
