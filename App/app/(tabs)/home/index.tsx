import { useEffect } from 'react'
import { useRouter } from 'expo-router'

export default function HomeScreen() {
  const router = useRouter()

  useEffect(() => {
    // Redirect to setup screen
    router.replace('/home/setup')
  }, [router])

  return null
}
