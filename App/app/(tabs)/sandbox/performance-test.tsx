import { useState } from 'react'
import {
  ActivityIndicator,
  Alert,
  ScrollView,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from 'react-native'

import { theme } from '@/constants/theme'
import { ChessEngine } from '@/lib/chess'

interface TestResult {
  iterativeMove: string
  iterativeTotalTime: number
  iterativeDepthTimes: number[]
  directMove: string
  directTotalTime: number
}

export default function PerformanceTestScreen() {
  const [isRunning, setIsRunning] = useState(false)
  const [result, setResult] = useState<TestResult | null>(null)
  const [debugMessage, setDebugMessage] = useState('')

  const runTest = async () => {
    console.log('Performance test: runTest called')
    setDebugMessage('Button was pressed! Starting test...')
    setIsRunning(true)
    setResult(null)

    try {
      // Test 1: Iterative Deepening (depths 1-5)
      console.log('TEST 1: Iterative Deepening (depths 1 → 5)')
      console.log('Creating engine for iterative test...')
      const engine1 = new ChessEngine()
      engine1.newGame()

      const aiVersion = 'v2'

      const iterativeStartTime = Date.now()
      let iterativeMove = ''
      const iterativeDepthTimes: number[] = []

      for (let depth = 1; depth <= 5; depth++) {
        const depthStartTime = Date.now()
        const move = await engine1.getBestMoveAtDepth(depth, 0, aiVersion)
        const depthTime = Date.now() - depthStartTime

        iterativeDepthTimes.push(depthTime)
        console.log(`  Depth ${depth}: ${move} (${depthTime}ms)`)

        if (depth === 5) {
          iterativeMove = move
        }
      }

      const iterativeTotalTime = Date.now() - iterativeStartTime
      console.log(`  Total time: ${iterativeTotalTime}ms`)

      // Test 2: Direct Depth 5 Search (fresh engine, no iterative deepening)
      console.log('TEST 2: Direct Depth 5 Search (no iterative deepening)')
      console.log('Creating fresh engine for direct test...')
      const engine2 = new ChessEngine()
      engine2.newGame()

      const directStartTime = Date.now()
      const directMove = await engine2.getBestMoveAtDepth(5, 0, aiVersion)
      const directTotalTime = Date.now() - directStartTime

      console.log(`  Depth 5: ${directMove} (${directTotalTime}ms)`)

      setResult({
        iterativeMove,
        iterativeTotalTime,
        iterativeDepthTimes,
        directMove,
        directTotalTime,
      })
      console.log('Performance test: Test completed successfully')
      setDebugMessage('Test completed successfully!')
    } catch (error) {
      console.error('Performance test: Test failed:', error)
      setDebugMessage(`Test failed: ${error}`)
      Alert.alert('Test Failed', `Error: ${error}`)
    } finally {
      console.log('Performance test: Cleaning up, setting isRunning to false')
      setIsRunning(false)
    }
  }

  const renderAnalysis = () => {
    if (!result) return null

    const difference = result.directTotalTime - result.iterativeTotalTime
    const percentage = ((Math.abs(difference) / result.directTotalTime) * 100).toFixed(1)
    const isFaster = result.iterativeTotalTime < result.directTotalTime
    const speedup = isFaster
      ? (result.directTotalTime / result.iterativeTotalTime).toFixed(2)
      : (result.iterativeTotalTime / result.directTotalTime).toFixed(2)

    return (
      <View style={styles.resultsContainer}>
        <Text style={styles.sectionTitle}>Iterative Deepening (depths 1-5)</Text>
        <View style={styles.resultBox}>
          {result.iterativeDepthTimes.map((time, index) => (
            <Text key={index} style={styles.depthText}>
              Depth {index + 1}: {time}ms
            </Text>
          ))}
          <Text style={[styles.resultText, styles.totalText]}>
            Total: {result.iterativeTotalTime}ms
          </Text>
          <Text style={styles.moveText}>Move: {result.iterativeMove}</Text>
        </View>

        <Text style={[styles.sectionTitle, styles.marginTop]}>Direct Depth 5</Text>
        <View style={styles.resultBox}>
          <Text style={styles.resultText}>Time: {result.directTotalTime}ms</Text>
          <Text style={styles.moveText}>Move: {result.directMove}</Text>
        </View>

        <Text style={[styles.sectionTitle, styles.marginTop]}>Analysis</Text>
        <View style={[styles.resultBox, isFaster ? styles.successBox : styles.warningBox]}>
          <Text style={[styles.analysisText, isFaster ? styles.successText : styles.warningText]}>
            {isFaster ? '✅ ' : '❌ '}
            Iterative deepening is {isFaster ? 'FASTER' : 'SLOWER'} by {Math.abs(difference)}ms
          </Text>
          <Text style={styles.percentageText}>
            {percentage}% {isFaster ? 'faster' : 'slower'} ({speedup}x{'  '}
            {isFaster ? 'speedup' : 'slowdown'})
          </Text>
          <Text style={styles.explanationText}>
            {isFaster
              ? 'Transposition table benefits outweigh the overhead of searching shallower depths'
              : 'Overhead of multiple searches exceeds transposition table benefits'}
          </Text>
        </View>

        <View style={[styles.resultBox, styles.marginTop]}>
          <Text style={styles.noteTitle}>Why Iterative Deepening Should Be Faster:</Text>
          <Text style={styles.noteText}>
            • Transposition table hits from shallower depths{'\n'}• Better move ordering using
            previous depth results{'\n'}• Aspiration windows (v2) using previous scores
          </Text>
        </View>
      </View>
    )
  }

  return (
    <View style={styles.container}>
      <ScrollView contentContainerStyle={styles.scrollContent}>
        <View style={styles.header}>
          <Text style={styles.title}>Iterative Deepening Performance Test</Text>
          <Text style={styles.subtitle}>
            Compares iterative deepening (depths 1-5) vs direct depth 5 search
          </Text>
          <Text style={styles.subtitle}>Position: Starting position (default)</Text>
          <Text style={styles.subtitle}>AI Version: v2</Text>
        </View>

        {debugMessage ? (
          <View style={styles.debugBox}>
            <Text style={styles.debugText}>{debugMessage}</Text>
          </View>
        ) : null}

        <TouchableOpacity
          style={[styles.runButton, isRunning && styles.runButtonDisabled]}
          onPress={runTest}
          disabled={isRunning}
        >
          {isRunning ? (
            <>
              <ActivityIndicator color="#fff" style={styles.spinner} />
              <Text style={styles.runButtonText}>Running Test...</Text>
            </>
          ) : (
            <Text style={styles.runButtonText}>Run Performance Test</Text>
          )}
        </TouchableOpacity>

        {renderAnalysis()}
      </ScrollView>
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
  },
  scrollContent: {
    padding: 16,
  },
  header: {
    marginBottom: 24,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: theme.colors.text.light,
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 14,
    color: theme.colors.secondary,
    marginBottom: 4,
  },
  debugBox: {
    backgroundColor: '#fff9c4',
    padding: 12,
    borderRadius: 8,
    marginBottom: 16,
    borderWidth: 2,
    borderColor: '#fbc02d',
  },
  debugText: {
    fontSize: 14,
    color: '#f57f17',
    fontWeight: '600',
  },
  runButton: {
    backgroundColor: theme.colors.primary,
    paddingHorizontal: 24,
    paddingVertical: 16,
    borderRadius: 8,
    alignItems: 'center',
    flexDirection: 'row',
    justifyContent: 'center',
    marginBottom: 24,
  },
  runButtonDisabled: {
    opacity: 0.6,
  },
  spinner: {
    marginRight: 8,
  },
  runButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  resultsContainer: {
    gap: 8,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '600',
    color: theme.colors.text.light,
    marginBottom: 8,
  },
  marginTop: {
    marginTop: 16,
  },
  resultBox: {
    backgroundColor: '#fff',
    padding: 16,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  successBox: {
    backgroundColor: '#e8f5e9',
    borderColor: '#4caf50',
  },
  warningBox: {
    backgroundColor: '#fff3e0',
    borderColor: '#ff9800',
  },
  depthText: {
    fontSize: 14,
    color: theme.colors.text.light,
    marginBottom: 4,
  },
  resultText: {
    fontSize: 16,
    color: theme.colors.text.light,
    marginBottom: 4,
  },
  totalText: {
    fontWeight: '600',
    fontSize: 18,
    marginTop: 8,
  },
  moveText: {
    fontSize: 14,
    color: theme.colors.secondary,
    marginTop: 4,
  },
  analysisText: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 8,
  },
  successText: {
    color: '#2e7d32',
  },
  warningText: {
    color: '#ef6c00',
  },
  percentageText: {
    fontSize: 16,
    color: theme.colors.text.light,
    marginBottom: 8,
  },
  explanationText: {
    fontSize: 14,
    color: theme.colors.secondary,
    fontStyle: 'italic',
  },
  noteTitle: {
    fontSize: 14,
    fontWeight: '600',
    color: theme.colors.text.light,
    marginBottom: 8,
  },
  noteText: {
    fontSize: 14,
    color: theme.colors.secondary,
    lineHeight: 20,
  },
})
