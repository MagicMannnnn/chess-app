import { useState, useCallback } from 'react'
import { ScrollView, StyleSheet, Text, View, TouchableOpacity } from 'react-native'
import Slider from '@react-native-community/slider'

import ChessBoard from '@/components/ChessBoard'
import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'

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
    paddingHorizontal: 16,
    paddingTop: 8,
    paddingBottom: 12,
  },
  barContainer: {
    height: 24,
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
    fontSize: 14,
    fontWeight: '600',
    color: theme.colors.text.light,
    textAlign: 'center',
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    marginTop: 4,
    gap: 12,
  },
  depthText: {
    fontSize: 12,
    fontWeight: '500',
    color: theme.colors.text.light,
    opacity: 0.7,
  },
})

export default function SandboxScreen() {
  const { settings } = useSettings()
  const [searchDepth, setSearchDepth] = useState(3)
  const [showBestMove, setShowBestMove] = useState(true)
  const [evaluation, setEvaluation] = useState(0)
  const [evalDepth, setEvalDepth] = useState(0)

  const handleEvaluationChange = useCallback((score: number, depth: number) => {
    setEvaluation(score)
    setEvalDepth(depth)
  }, [])

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
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

        <View style={styles.controlRow}>
          <Text style={styles.label}>Show Best Move</Text>
          <TouchableOpacity style={styles.toggle} onPress={() => setShowBestMove(!showBestMove)}>
            <View style={[styles.toggleInner, showBestMove && styles.toggleActive]}>
              <Text style={styles.toggleText}>{showBestMove ? 'ON' : 'OFF'}</Text>
            </View>
          </TouchableOpacity>
        </View>
      </View>

      <ChessBoard
        autoFlip={settings.autoFlipBoard}
        showBestMove={showBestMove}
        searchDepth={searchDepth}
        onEvaluationChange={handleEvaluationChange}
      />
    </ScrollView>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
  },
  content: {
    paddingTop: 4,
    paddingBottom: 8,
  },
  controls: {
    paddingHorizontal: 16,
    paddingBottom: 8,
    gap: 8,
  },
  controlRow: {
    gap: 4,
  },
  label: {
    fontSize: 16,
    fontWeight: '600',
    color: theme.colors.text.primary,
  },
  slider: {
    width: '100%',
    height: 32,
  },
  toggle: {
    width: 80,
    height: 40,
    borderRadius: 20,
    backgroundColor: theme.colors.background.dark,
    justifyContent: 'center',
    paddingHorizontal: 4,
  },
  toggleInner: {
    width: 36,
    height: 32,
    borderRadius: 16,
    backgroundColor: theme.colors.background.medium,
    justifyContent: 'center',
    alignItems: 'center',
  },
  toggleActive: {
    backgroundColor: theme.colors.primary,
    alignSelf: 'flex-end',
  },
  toggleText: {
    fontSize: 12,
    fontWeight: '600',
    color: '#fff',
  },
})
