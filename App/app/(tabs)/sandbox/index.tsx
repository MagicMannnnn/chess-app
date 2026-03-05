import { StyleSheet, View } from 'react-native'

import ChessBoard from '@/components/ChessBoard'
import { theme } from '@/constants/theme'
import { useSettings } from '@/contexts/SettingsContext'

export default function SandboxScreen() {
  const { settings } = useSettings()

  return (
    <View style={styles.container}>
      <ChessBoard autoFlip={settings.autoFlipBoard} />
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background.light,
  },
})
